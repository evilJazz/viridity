#include "rewriterequesthandler.h"

RewriteRequestHandler::RewriteRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

RewriteRequestHandler::~RewriteRequestHandler()
{
}

void RewriteRequestHandler::addRule(const QString &rule)
{
    rewriteRules_.append(rule);
}

void RewriteRequestHandler::removeRule(const QString &rule)
{
    rewriteRules_.removeOne(rule);
}

void RewriteRequestHandler::clear()
{
    rewriteRules_.clear();
}

bool RewriteRequestHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    QString url = request->url();

    foreach (const QString &rule, rewriteRules_)
    {
        QStringList ruleParts = rule.trimmed().split(" ");

        if (ruleParts.count() > 1)
        {
            QRegExp re(ruleParts.at(0));

            if (re.indexIn(url) > -1)
            {
                QString ruleRewrite = ruleParts.at(1);

                QStringList capturedTexts = re.capturedTexts();
                for (int i = 0; i < capturedTexts.length(); ++i)
                {
                    QString cap = capturedTexts.at(i);
                    ruleRewrite.replace("$" + QString::number(i), cap);
                }

                qDebug("Rewriting url %s to %s", request->url().constData(), ruleRewrite.toLatin1().constData());

                request->rewriteUrl(ruleRewrite.toLatin1());
            }
        }
    }
}

void RewriteRequestHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    // Does not handle any request itself...
}
