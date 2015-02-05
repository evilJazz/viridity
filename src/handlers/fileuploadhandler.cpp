#include "fileuploadhandler.h"

#include "viriditywebserver.h"

#include <QFile>
#include <QDir>
#include "KCL/filesystemutils.h"

/* FileUploadDataHandler */

class FileUploadDataHandler : public QObject
{
    Q_OBJECT
public:
    explicit FileUploadDataHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, ViriditySession *session, QObject *parent = 0) :
        QObject(parent),
        request_(request),
        response_(response),
        session_(session),
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
                response_->writeHead(Tufao::HttpServerResponse::BAD_REQUEST);
                response_->end();
            }

            boundary_ = r.cap(1).toUtf8();
        }

        connect(request_, SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
        connect(request_, SIGNAL(end()), this, SLOT(onEnd()));
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
                int endIndex = analyzeBuffer_.length() - 1;

                if (isComplete)
                    endIndex = findEndIndex(analyzeBuffer_, endIndex);

                QByteArray start = analyzeBuffer_.mid(pos, endIndex - pos + 1);

                ++fileId_;
                targetFileName_ = FileSystemUtils::pathAppend(QDir::tempPath(), "temp_" + QString::number(fileId_));
                targetFile_.setFileName(targetFileName_);

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
        //parts_.append(data_);
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
            fileNames_.append(targetFile_.fileName());
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
        response_->writeHead(Tufao::HttpServerResponse::OK);
        response_->end();
    }

private:
    Tufao::HttpServerRequest *request_;
    Tufao::HttpServerResponse *response_;
    ViriditySession *session_;
    QByteArray analyzeBuffer_;

    QByteArray contentType_;
    bool isMultiPart_;
    QByteArray boundary_;

    QFile targetFile_;
    QString targetFileName_;

    QStringList fileNames_;
    int fileId_;
};


/* FileUploadHandler */

FileUploadHandler::FileUploadHandler(ViridityConnection *parent) :
    QObject(parent),
    connection_(parent)
{
}

bool FileUploadHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QUrl url(request->url());
    QString filename = url.path();
    return filename == "/upload";
}

void FileUploadHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QString id = UrlQuery(request->url()).queryItemValue("id");

    ViriditySession *session = connection_->server()->sessionManager()->getSession(id);

    if (session)
    {
        if (request->method() == "POST")
        {
            // Hand off to separate data handler!
            FileUploadDataHandler *handler = new FileUploadDataHandler(request, response, session);
            connect(response, SIGNAL(destroyed()), handler, SLOT(deleteLater()));
        }
    }
}

#include "fileuploadhandler.moc" // for FileUploadDataHandler
