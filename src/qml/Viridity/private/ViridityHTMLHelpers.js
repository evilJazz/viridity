.pragma library

function getHTMLDocument(item)
{
    var topLevelRenderer = item.topLevelRenderer;
    var document = null;

    if (topLevelRenderer.hasOwnProperty("isHTMLDocument")) // ViridityHTMLDocument
        document = topLevelRenderer;
    else if (topLevelRenderer.hasOwnProperty("targetHTMLDocument")) // ViriditiyHTMLSessionSegment
        document = topLevelRenderer.targetHTMLDocument;

    return document;
}
