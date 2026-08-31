import Foundation

enum Firmware: String, CaseIterable {
    case colecoVision = "colecovision.rom"

    var title: String {
        L10n("Settings::ColecoVisionBIOS")
    }

    var expectedSize: Int {
        0x2000
    }

    var validationErrorMessage: String {
        L10n("Settings::ColecoVisionBIOSSize")
    }
}
