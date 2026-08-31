import Foundation

enum AppConfiguration {
    static let libraryTitleLocalizationKey = "Common::Gearcoleco"
    static let thumbnailBaseURL = URL(string: "https://www.drhelius.com/thumbnails/gearcoleco/")!

    static func romCRC(inArchiveAt url: URL) -> String? {
        GearcolecoEmulator.romCRC(inArchiveAt: url)
    }
}
