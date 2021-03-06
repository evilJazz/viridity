/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

#include "fileuploadhandler.h"

#include "viriditywebserver.h"

#include <QFile>
#include <QDir>
#include "KCL/filesystemutils.h"

#include "viriditysessionmanager.h"

/* FileUploadDataHandler */

class FileUploadDataHandler : public QObject
{
    Q_OBJECT
public:
    explicit FileUploadDataHandler(FileUploadHandler *parent, QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response) :
        QObject(),
        parent_(parent),
        request_(request),
        response_(response),
        fileId_(0)
    {
        QUrl url(request->url());
        QString filename = url.path();

        contentType_ = request->headers().value("Content-Type");
        isMultiPart_ = QString(contentType_).startsWith("multipart/", Qt::CaseInsensitive);

        if (isMultiPart_)
        {
            QRegExp r(".*boundary=\"?([^\"]+)\"?\\s?", Qt::CaseInsensitive);

            if (r.indexIn(QString::fromUtf8(contentType_)) != 0)
            {
                response_->writeHead(ViridityHttpServerResponse::BAD_REQUEST);
                response_->end();
            }

            boundary_ = r.cap(1).toUtf8();
        }

        connect(request_.data(), SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
        connect(request_.data(), SIGNAL(end()), this, SLOT(onEnd()));
    }

private slots:

    QList<QByteArray> splitChunk(QByteArray chunk)
    {
        QList<QByteArray> result;

        int boundaryPos = chunk.indexOf(boundary_);

        // Only return at least one item if we found a boundary in the current chunk...
        if (boundaryPos > -1)
        {
            while (boundaryPos > -1)
            {
                QByteArray split = chunk.left(boundaryPos);

                result.append(split);

                int newStartPos = boundaryPos + boundary_.length();

                if (chunk.at(newStartPos) == '\r')
                    ++newStartPos;

                if (chunk.at(newStartPos) == '\n')
                    ++newStartPos;

                chunk = chunk.mid(newStartPos);

                boundaryPos = chunk.indexOf(boundary_);
            }

            // Return left-overs...
            result.append(chunk);
        }

        return result;
    }

    void writeDataToCurrentPart(const QByteArray &newData, bool isComplete = false)
    {
        // Only add newData to the analyze buffer if size below 1 KiB to avoid buffer overflow attacks.
        if (!targetFile_.isWritable() && analyzeBuffer_.size() < 1024)
        {
            analyzeBuffer_ += newData;

            int pos = 0;
            while (pos < analyzeBuffer_.size() - 1)
            {
                if (analyzeBuffer_.at(pos) == '\n' && (analyzeBuffer_.at(pos + 1) == '\n' || analyzeBuffer_.at(pos + 1) == '\r'))
                {
                    if (analyzeBuffer_.at(pos + 1) == '\r')
                        pos++;

                    pos += 2;
                    break;
                }
                pos++;
            }

            if (pos < analyzeBuffer_.size())
            {
                // Read headers...
                currentPart_.headerFields.clear();

                QString header = QString::fromUtf8(analyzeBuffer_.left(pos));
                QStringList headerLines = header.split('\n');

                foreach (const QString &headerLine, headerLines)
                {
                    QStringList lineParts = headerLine.split(':');
                    if (lineParts.count() == 2)
                    {
                        QString key = lineParts[0].trimmed();
                        QString value = lineParts[1].trimmed();

                        if (key.compare(key, "Content-Disposition", Qt::CaseInsensitive) == 0)
                        {
                            currentPart_.name = readSubValueFromHeaderValue("name", value);
                            currentPart_.fileName = readSubValueFromHeaderValue("filename", value);

                            sanitizeFileName(currentPart_.name);
                            sanitizeFileName(currentPart_.fileName);

                            currentPart_.originalFileName = currentPart_.fileName;
                        }

                        currentPart_.headerFields.insert(key, value);
                    }
                }

                // Set temp filename...
                ++fileId_;

                currentPart_.tempFileName = FileSystemUtils::pathAppend(QDir::tempPath(), "temp_" + QString::number(fileId_) + "_" + currentPart_.fileName);

                if (filenameToParts_.contains(currentPart_.fileName) || QFile::exists(currentPart_.tempFileName))
                {
                     currentPart_.fileName = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + currentPart_.fileName;
                     currentPart_.tempFileName = FileSystemUtils::pathAppend(QDir::tempPath(), "temp_" + QString::number(fileId_) + "_" + currentPart_.fileName);
                }

                targetFile_.setFileName(currentPart_.tempFileName);

                // Write beginning to file...
                int endIndex = analyzeBuffer_.length() - 1;

                if (isComplete)
                    endIndex = findEndIndex(analyzeBuffer_, endIndex);

                QByteArray start = analyzeBuffer_.mid(pos, endIndex - pos + 1);

                if (targetFile_.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    targetFile_.write(start);

                if (isComplete)
                    finishCurrentPart();
            }
        }
        else if (targetFile_.isWritable())
        {
            targetFile_.write(newData);
        }
    }

    static QString readSubValueFromHeaderValue(const QString &key, const QString &input)
    {
        QRegExp r(".*[\\s;]+" + key + "=\"?([^\"]+)\"?\\s?", Qt::CaseInsensitive);

        if (r.indexIn(input) != 0)
            return QString::null;
        else
            return r.cap(1);
    }

    static void sanitizeFileName(QString &input)
    {
        input.replace(QRegExp("[,+&/=:#!?_@$*|\\\\\"'\\^~`]"), "");
        input = input.simplified();
    }

    int findEndIndex(const QByteArray &data, int endIndex)
    {
        int newEndIndex = endIndex;

        if (data.at(newEndIndex) == '-' && data.at(newEndIndex - 1) == '-')
            newEndIndex -= 2;

        if (data.at(newEndIndex) == '\n')
            --newEndIndex;

        if (data.at(newEndIndex) == '\r')
            --newEndIndex;

        return newEndIndex;
    }

    void writeLastBlockOfCurrentPart(const QByteArray &newData)
    {
        if (targetFile_.isWritable())
        {
            int newEndPos = findEndIndex(newData, newData.length() - 1);

            QByteArray end = newData.left(newEndPos + 1);

            targetFile_.write(end);
            finishCurrentPart();
        }
    }

    void finishCurrentPart()
    {
        if (targetFile_.isWritable())
        {
            targetFile_.close();
            parts_.append(currentPart_);
            filenameToParts_.insert(currentPart_.fileName, &parts_.last());
            analyzeBuffer_.clear();
        }
    }

    void onData(const QByteArray &chunk)
    {
        QList<QByteArray> splits = splitChunk(chunk);

        if (splits.count() == 0)
            writeDataToCurrentPart(chunk);
        else
        {
            QByteArray trim = splits.first().trimmed();
            if (trim == "--" || trim.isEmpty())
                splits.removeFirst();
            else if (splits.count() > 1)
            {
                writeLastBlockOfCurrentPart(splits.takeFirst());
            }

            analyzeBuffer_.clear();

            trim = splits.last().trimmed();
            if (trim == "--" || trim.isEmpty())
                splits.removeLast();
            else
            {
                writeDataToCurrentPart(splits.takeLast());
            }

            foreach (const QByteArray &split, splits)
            {
                writeDataToCurrentPart(split, true);
            }
        }
    }

    void onEnd()
    {
        /*
        if (!analyzeBuffer_.isEmpty())
        {
            finishCurrentPart();
            analyzeBuffer_.clear();
        }
        */

        // handle request
        response_->headers().insert("Content-Type", "text/plain");
        response_->writeHead(ViridityHttpServerResponse::OK);
        response_->end();

        emitResults();
    }

    void emitResults()
    {
        QVariantList result;

        foreach (const Part &part, parts_)
        {
            QVariantMap file;

            file.insert("originalFileName", part.originalFileName);
            file.insert("fileName", part.fileName);
            file.insert("name", part.name);
            file.insert("tempFileName", part.tempFileName);
            file.insert("mimeType", part.mimeType);

            result.append(file);
        }

        emit parent_->newFilesUploaded(result);

        /*
        foreach (const Part &part, parts_)
        {
            if (QFile::exists(part.tempFileName))
                QFile::remove(part.tempFileName);
        }
        */

        parts_.clear();
        filenameToParts_.clear();

        if (request_)
        {
            disconnect(request_.data(), SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
            disconnect(request_.data(), SIGNAL(end()), this, SLOT(onEnd()));
        }

        deleteLater();
    }

private:
    FileUploadHandler *parent_;
    QSharedPointer<ViridityHttpServerRequest> request_;
    QSharedPointer<ViridityHttpServerResponse> response_;
    QByteArray analyzeBuffer_;

    QByteArray contentType_;
    bool isMultiPart_;
    QByteArray boundary_;

    int fileId_;

    struct Part
    {
        QString name;
        QString originalFileName;
        QString fileName;
        QString mimeType;
        QString tempFileName;
        QHash<QString, QString> headerFields;
    };

    QList<Part> parts_;
    QHash<QString, Part *> filenameToParts_;

    QFile targetFile_;
    Part currentPart_;
};


/* FileUploadHandler */

FileUploadHandler::FileUploadHandler(const QString &urlEndPoint, ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    urlEndPoint_(urlEndPoint)
{
}

FileUploadHandler::~FileUploadHandler()
{
}

bool FileUploadHandler::doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request)
{
    QRegExp urlRe(urlEndPoint_);
    return QString::fromUtf8(request->url()).indexOf(urlRe) > -1;
}

void FileUploadHandler::handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    if (request->method() == "POST")
    {
        // Hand off to separate data handler!
        FileUploadDataHandler *handler = new FileUploadDataHandler(this, request, response);
        connect(response.data(), SIGNAL(destroyed()), handler, SLOT(deleteLater()));
        return;
    }
    else if (request->method() == "OPTIONS") // to allow CORS pre check
    {
        response->writeHead(200);
        response->end("OK");
        return;
    }

    response->writeHead(405);
    response->end("File missing.");
}

#include "fileuploadhandler.moc" // for FileUploadDataHandler
