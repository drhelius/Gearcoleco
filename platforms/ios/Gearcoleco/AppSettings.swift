import Foundation

enum GearcolecoTimingOption: Int, CaseIterable {
    case automatic
    case ntsc
    case pal

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .ntsc: return "NTSC (60 Hz)"
        case .pal: return "PAL (50 Hz)"
        }
    }
}

enum GearcolecoMapperOption: Int, CaseIterable {
    case automatic
    case standard
    case megaCart
    case activision
    case ocm

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .standard: return L10n("Settings::Standard")
        case .megaCart: return "MegaCart"
        case .activision: return "Activision"
        case .ocm: return "OCM"
        }
    }
}

enum GearcolecoVideoChipOption: Int, CaseIterable {
    case automatic
    case tms9918A
    case f18A

    var title: String {
        switch self {
        case .automatic: return L10n("Settings::Automatic")
        case .tms9918A: return "TMS9918A"
        case .f18A: return "F18A"
        }
    }
}

enum GearcolecoPaletteOption: Int, CaseIterable {
    case coleco
    case tms9918

    var title: String {
        switch self {
        case .coleco: return "Coleco"
        case .tms9918: return "TMS9918"
        }
    }
}

enum GearcolecoOverscanOption: Int, CaseIterable {
    case disabled
    case topBottom
    case full284
    case full320

    var title: String {
        switch self {
        case .disabled: return L10n("Settings::Disabled")
        case .topBottom: return "Top + Bottom"
        case .full284: return "Full (284 width)"
        case .full320: return "Full (320 width)"
        }
    }
}

enum AppSettings {
    private enum Key {
        static let audioEnabled = "settings.audioEnabled"
        static let hapticsEnabled = "settings.hapticsEnabled"
        static let smoothingEnabled = "settings.smoothingEnabled"
        static let screenSize = "settings.screenSize"
        static let timing = "settings.timing"
        static let mapper = "settings.mapper"
        static let videoChip = "settings.videoChip"
        static let palette = "settings.palette"
        static let overscan = "settings.overscan"
        static let noSpriteLimitEnabled = "settings.noSpriteLimitEnabled"
        static let saveStateSlot = "settings.saveStateSlot"
    }

    static func registerDefaults() {
        UserDefaults.standard.register(defaults: [
            Key.audioEnabled: true,
            Key.hapticsEnabled: true,
            Key.smoothingEnabled: false,
            Key.screenSize: ScreenSizeOption.fitToWidth.rawValue,
            Key.timing: GearcolecoTimingOption.automatic.rawValue,
            Key.mapper: GearcolecoMapperOption.automatic.rawValue,
            Key.videoChip: GearcolecoVideoChipOption.automatic.rawValue,
            Key.palette: GearcolecoPaletteOption.coleco.rawValue,
            Key.overscan: GearcolecoOverscanOption.disabled.rawValue,
            Key.noSpriteLimitEnabled: false,
            Key.saveStateSlot: 1
        ])
    }

    static var audioEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.audioEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.audioEnabled) }
    }

    static var hapticsEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.hapticsEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.hapticsEnabled) }
    }

    static var smoothingEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.smoothingEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.smoothingEnabled) }
    }

    static var screenSize: ScreenSizeOption {
        get { ScreenSizeOption(rawValue: UserDefaults.standard.integer(forKey: Key.screenSize)) ?? .fitToWidth }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.screenSize) }
    }

    static var timing: GearcolecoTimingOption {
        get { GearcolecoTimingOption(rawValue: UserDefaults.standard.integer(forKey: Key.timing)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.timing) }
    }

    static var mapper: GearcolecoMapperOption {
        get { GearcolecoMapperOption(rawValue: UserDefaults.standard.integer(forKey: Key.mapper)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.mapper) }
    }

    static var videoChip: GearcolecoVideoChipOption {
        get { GearcolecoVideoChipOption(rawValue: UserDefaults.standard.integer(forKey: Key.videoChip)) ?? .automatic }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.videoChip) }
    }

    static var palette: GearcolecoPaletteOption {
        get { GearcolecoPaletteOption(rawValue: UserDefaults.standard.integer(forKey: Key.palette)) ?? .coleco }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.palette) }
    }

    static var overscan: GearcolecoOverscanOption {
        get { GearcolecoOverscanOption(rawValue: UserDefaults.standard.integer(forKey: Key.overscan)) ?? .disabled }
        set { UserDefaults.standard.set(newValue.rawValue, forKey: Key.overscan) }
    }

    static var noSpriteLimitEnabled: Bool {
        get { UserDefaults.standard.bool(forKey: Key.noSpriteLimitEnabled) }
        set { UserDefaults.standard.set(newValue, forKey: Key.noSpriteLimitEnabled) }
    }

    static var saveStateSlot: Int {
        get { min(max(UserDefaults.standard.integer(forKey: Key.saveStateSlot), 1), 5) }
        set { UserDefaults.standard.set(min(max(newValue, 1), 5), forKey: Key.saveStateSlot) }
    }
}
