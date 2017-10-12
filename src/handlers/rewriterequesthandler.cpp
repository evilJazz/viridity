/****************************************************************************
**
** Copyright (C) 2012-2017 Andre Beckedorf, Meteora Softworks
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

#include "rewriterequesthandler.h"

#include "KCL/debug.h"

bool RewriteRequestHandler::RewriteRule::interpretRule(const QString &rule)
{
    QStringList ruleParts = rule.trimmed().split(" ");

    if (ruleParts.count() > 1)
    {
        this->pattern = ruleParts.at(0);
        this->substitution = ruleParts.at(1);

        if (ruleParts.count() > 2)
        {
            if (ruleParts.at(2).startsWith("[R"))
            {
                this->flag = RewriteExternal;

                QRegExp re("=([0-9]{3})\\]");
                if (re.indexIn(ruleParts.at(2)) > -1)
                    this->rewriteHttpStatusCode = re.capturedTexts().at(1).toInt();
                else
                    this->rewriteHttpStatusCode = 301;
            }
        }
        else
            this->flag = RewriteInternal;

        return true;
    }

    return false;
}

QString RewriteRequestHandler::RewriteRule::doesMatchAndSubstitute(const QString &input) const
{
    QRegExp re(this->pattern);

    if (re.indexIn(input) > -1)
    {
        QString ruleRewrite = this->substitution;

        QStringList capturedTexts = re.capturedTexts();
        for (int i = 0; i < capturedTexts.length(); ++i)
        {
            QString cap = capturedTexts.at(i);
            ruleRewrite.replace("$" + QString::number(i), cap);
        }

        return ruleRewrite;
    }

    return QString::null;
}

RewriteRequestHandler::RewriteRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

RewriteRequestHandler::~RewriteRequestHandler()
{
}

bool RewriteRequestHandler::addRule(const QString &rule)
{
    RewriteRule irule;
    if (irule.interpretRule(rule))
    {
        interpretedRewriteRules_.append(irule);
        rewriteRules_.append(rule);
        return true;
    }

    return false;
}

bool RewriteRequestHandler::removeRule(const QString &rule)
{
    int index = rewriteRules_.indexOf(rule);

    if (index > -1)
    {
        rewriteRules_.removeAt(index);
        interpretedRewriteRules_.removeAt(index);
        return true;
    }

    return false;
}

void RewriteRequestHandler::clear()
{
    rewriteRules_.clear();
}

bool RewriteRequestHandler::doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request)
{
    QString url = request->url();

    foreach (const RewriteRule &rule, interpretedRewriteRules_)
    {
        if (rule.flag == RewriteInternal)
        {
            QString substitutedUrl = rule.doesMatchAndSubstitute(url);

            if (!substitutedUrl.isNull())
            {
                DPRINTF("Internally rewriting url %s to %s", request->url().constData(), substitutedUrl.toLatin1().constData());

                request->rewriteUrl(substitutedUrl.toLatin1());
            }
        }
        else if (rule.flag == RewriteExternal)
        {
            QString substitutedUrl = rule.doesMatchAndSubstitute(url);

            if (!substitutedUrl.isNull())
                return true;
        }
    }

    return false;
}

void RewriteRequestHandler::handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    QString url = request->url();

    foreach (const RewriteRule &rule, interpretedRewriteRules_)
    {
        if (rule.flag == RewriteExternal)
        {
            QString substitutedUrl = rule.doesMatchAndSubstitute(url);

            if (!substitutedUrl.isNull())
            {
                DPRINTF("Externally rewriting url %s to %s", request->url().constData(), substitutedUrl.toLatin1().constData());

                response->headers().insert("Location", substitutedUrl.toLatin1());
                response->writeHead(rule.rewriteHttpStatusCode);
                response->end();
            }
        }
    }
}

