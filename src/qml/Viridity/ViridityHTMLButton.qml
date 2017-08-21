/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    template: '<button onclick="' + topLevelTemplateRenderer.targetId + '">${title}</button>';

    property string title
    signal clicked()
}
