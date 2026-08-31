import UIKit

final class GameControlsView: UIView {
    var onButtonChanged: ((GearcolecoButton, Bool) -> Void)? {
        didSet {
            actionYellow.onButtonChanged = onButtonChanged
            actionRed.onButtonChanged = onButtonChanged
            actionBlue.onButtonChanged = onButtonChanged
            actionPurple.onButtonChanged = onButtonChanged
            keypad.onButtonChanged = onButtonChanged
        }
    }

    var hapticsEnabled = true {
        didSet {
            dPad.hapticsEnabled = hapticsEnabled
            actionYellow.hapticsEnabled = hapticsEnabled
            actionRed.hapticsEnabled = hapticsEnabled
            actionBlue.hapticsEnabled = hapticsEnabled
            actionPurple.hapticsEnabled = hapticsEnabled
            keypad.hapticsEnabled = hapticsEnabled
        }
    }

    let dPad = DirectionPadView()
    let actionYellow = GameControlButton(title: "Y", button: .yellow, shape: .circle, accentColor: .systemYellow)
    let actionRed = GameControlButton(title: "R", button: .red, shape: .circle, accentColor: .systemRed)
    let actionBlue = GameControlButton(title: "B", button: .blue, shape: .circle, accentColor: .systemBlue)
    let actionPurple = GameControlButton(title: "P", button: .purple, shape: .circle, accentColor: .systemPurple)
    let keypad = ColecoKeypadView()

    private var portraitConstraints = [NSLayoutConstraint]()
    private var landscapeConstraints = [NSLayoutConstraint]()
    private var usingLandscapeConstraints = false

    override init(frame: CGRect) {
        super.init(frame: frame)
        configure()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        configure()
    }

    override func layoutSubviews() {
        let landscape = bounds.width > bounds.height
        if landscape != usingLandscapeConstraints {
            usingLandscapeConstraints = landscape
            NSLayoutConstraint.deactivate(landscape ? portraitConstraints : landscapeConstraints)
            NSLayoutConstraint.activate(landscape ? landscapeConstraints : portraitConstraints)
        }

        super.layoutSubviews()
    }

    private func configure() {
        isMultipleTouchEnabled = true
        backgroundColor = .clear
        dPad.onDirectionChanged = { [weak self] direction, pressed in
            guard let self else { return }
            self.onButtonChanged?(self.emulatorButton(for: direction), pressed)
        }

        dPad.translatesAutoresizingMaskIntoConstraints = false
        actionYellow.translatesAutoresizingMaskIntoConstraints = false
        actionRed.translatesAutoresizingMaskIntoConstraints = false
        actionBlue.translatesAutoresizingMaskIntoConstraints = false
        actionPurple.translatesAutoresizingMaskIntoConstraints = false
        keypad.translatesAutoresizingMaskIntoConstraints = false

        addSubview(dPad)
        addSubview(actionYellow)
        addSubview(actionRed)
        addSubview(actionBlue)
        addSubview(actionPurple)
        addSubview(keypad)

        let isPad = UIDevice.current.userInterfaceIdiom == .pad
        let portraitDPadSize: CGFloat = isPad ? 176.0 : 132.0
        let landscapeDPadSize: CGFloat = isPad ? 148.0 : 104.0
        let actionSize: CGFloat = isPad ? 72.0 : 58.0
        let actionSpacing: CGFloat = isPad ? 14.0 : 10.0
        let portraitKeypadWidth: CGFloat = isPad ? 180.0 : 144.0
        let portraitKeypadHeight: CGFloat = isPad ? 232.0 : 184.0
        let landscapeKeypadWidth: CGFloat = isPad ? 168.0 : 132.0
        let landscapeKeypadHeight: CGFloat = isPad ? 216.0 : 170.0
        let portraitBottomInset: CGFloat = isPad ? 24.0 : 8.0
        let landscapeBottomInset: CGFloat = isPad ? 20.0 : 8.0
        let keypadSpacing: CGFloat = isPad ? 16.0 : 12.0

        NSLayoutConstraint.activate([
            actionRed.widthAnchor.constraint(equalToConstant: actionSize),
            actionRed.heightAnchor.constraint(equalTo: actionRed.widthAnchor),

            actionYellow.trailingAnchor.constraint(equalTo: actionRed.leadingAnchor, constant: -actionSpacing),
            actionYellow.centerYAnchor.constraint(equalTo: actionRed.centerYAnchor),
            actionYellow.widthAnchor.constraint(equalTo: actionRed.widthAnchor),
            actionYellow.heightAnchor.constraint(equalTo: actionRed.heightAnchor),

            actionBlue.trailingAnchor.constraint(equalTo: actionRed.trailingAnchor),
            actionBlue.topAnchor.constraint(equalTo: actionRed.bottomAnchor, constant: actionSpacing),
            actionBlue.widthAnchor.constraint(equalTo: actionRed.widthAnchor),
            actionBlue.heightAnchor.constraint(equalTo: actionRed.heightAnchor),

            actionPurple.trailingAnchor.constraint(equalTo: actionYellow.trailingAnchor),
            actionPurple.centerYAnchor.constraint(equalTo: actionBlue.centerYAnchor),
            actionPurple.widthAnchor.constraint(equalTo: actionRed.widthAnchor),
            actionPurple.heightAnchor.constraint(equalTo: actionRed.heightAnchor)
        ])

        portraitConstraints = [
            dPad.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 20.0),
            actionRed.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -20.0),
            dPad.widthAnchor.constraint(equalToConstant: portraitDPadSize),
            dPad.heightAnchor.constraint(equalTo: dPad.widthAnchor),
            dPad.bottomAnchor.constraint(equalTo: keypad.topAnchor, constant: -keypadSpacing),
            actionBlue.bottomAnchor.constraint(equalTo: dPad.bottomAnchor),
            keypad.centerXAnchor.constraint(equalTo: safeAreaLayoutGuide.centerXAnchor),
            keypad.widthAnchor.constraint(equalToConstant: portraitKeypadWidth),
            keypad.heightAnchor.constraint(equalToConstant: portraitKeypadHeight),
            keypad.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -portraitBottomInset)
        ]

        landscapeConstraints = [
            dPad.leadingAnchor.constraint(equalTo: safeAreaLayoutGuide.leadingAnchor, constant: 8.0),
            actionRed.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -8.0),
            dPad.widthAnchor.constraint(equalToConstant: landscapeDPadSize),
            dPad.heightAnchor.constraint(equalTo: dPad.widthAnchor),
            dPad.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: 64.0),
            actionRed.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: 64.0),
            keypad.trailingAnchor.constraint(equalTo: safeAreaLayoutGuide.trailingAnchor, constant: -8.0),
            keypad.widthAnchor.constraint(equalToConstant: landscapeKeypadWidth),
            keypad.heightAnchor.constraint(equalToConstant: landscapeKeypadHeight),
            keypad.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -landscapeBottomInset)
        ]

        NSLayoutConstraint.activate(portraitConstraints)
    }

    private func emulatorButton(for direction: DirectionPadDirection) -> GearcolecoButton {
        switch direction {
        case .up: return .up
        case .down: return .down
        case .left: return .left
        case .right: return .right
        }
    }
}

final class ColecoKeypadView: UIView {
    var onButtonChanged: ((GearcolecoButton, Bool) -> Void)? {
        didSet {
            for button in buttons {
                button.onButtonChanged = onButtonChanged
            }
        }
    }

    var hapticsEnabled = true {
        didSet {
            for button in buttons {
                button.hapticsEnabled = hapticsEnabled
            }
        }
    }

    private let keys: [(String, GearcolecoButton)] = [
        ("1", .keypad1), ("2", .keypad2), ("3", .keypad3),
        ("4", .keypad4), ("5", .keypad5), ("6", .keypad6),
        ("7", .keypad7), ("8", .keypad8), ("9", .keypad9),
        ("*", .keypadAsterisk), ("0", .keypad0), ("#", .keypadHash)
    ]
    private var buttons = [GameControlButton]()

    override init(frame: CGRect) {
        super.init(frame: frame)
        configure()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        configure()
    }

    private func configure() {
        isMultipleTouchEnabled = true
        accessibilityLabel = L10n("Gameplay::ColecoKeypad")
        accessibilityIdentifier = "colecoKeypad"

        let spacing: CGFloat = UIDevice.current.userInterfaceIdiom == .pad ? 8.0 : 6.0
        let rows = stride(from: 0, to: keys.count, by: 3).map { startIndex -> UIStackView in
            let rowButtons = keys[startIndex..<(startIndex + 3)].map { title, emulatorButton -> UIView in
                let button = GameControlButton(title: title, button: emulatorButton, shape: .rounded)
                button.onButtonChanged = onButtonChanged
                button.hapticsEnabled = hapticsEnabled
                buttons.append(button)
                return button
            }
            let row = UIStackView(arrangedSubviews: rowButtons)
            row.axis = .horizontal
            row.spacing = spacing
            row.distribution = .fillEqually
            return row
        }

        let grid = UIStackView(arrangedSubviews: rows)
        grid.translatesAutoresizingMaskIntoConstraints = false
        grid.axis = .vertical
        grid.spacing = spacing
        grid.distribution = .fillEqually
        addSubview(grid)

        NSLayoutConstraint.activate([
            grid.topAnchor.constraint(equalTo: topAnchor),
            grid.bottomAnchor.constraint(equalTo: bottomAnchor),
            grid.leadingAnchor.constraint(equalTo: leadingAnchor),
            grid.trailingAnchor.constraint(equalTo: trailingAnchor)
        ])
    }
}

final class GameControlButton: UIButton {
    enum Shape {
        case circle
        case capsule
        case rounded
    }

    var onButtonChanged: ((GearcolecoButton, Bool) -> Void)?
    var hapticsEnabled = true

    private let emulatorButton: GearcolecoButton
    private let shape: Shape
    private let accentColor: UIColor?
    private var pressed = false
    private let feedback = UIImpactFeedbackGenerator(style: .light)

    init(title: String, button: GearcolecoButton, shape: Shape, accentColor: UIColor? = nil) {
        self.emulatorButton = button
        self.shape = shape
        self.accentColor = accentColor
        super.init(frame: .zero)

        setTitle(title, for: .normal)
        setTitleColor(accentColor ?? .label, for: .normal)
        titleLabel?.font = shape == .circle
            ? .systemFont(ofSize: 22.0, weight: .bold)
            : .systemFont(ofSize: 17.0, weight: .semibold)
        backgroundColor = UIColor.secondarySystemFill.withAlphaComponent(0.92)
        layer.borderColor = (accentColor ?? UIColor.separator).withAlphaComponent(0.7).cgColor
        layer.borderWidth = 1.0
        accessibilityLabel = title

        addTarget(self, action: #selector(press), for: [.touchDown, .touchDragEnter])
        addTarget(self, action: #selector(releaseButton), for: [.touchUpInside, .touchUpOutside, .touchCancel, .touchDragExit])
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func layoutSubviews() {
        super.layoutSubviews()

        switch shape {
        case .circle:
            layer.cornerRadius = bounds.width * 0.5
        case .capsule:
            layer.cornerRadius = bounds.height * 0.5
        case .rounded:
            layer.cornerRadius = 12.0
        }
    }

    @objc private func press() {
        guard !pressed else { return }
        pressed = true
        if hapticsEnabled {
            feedback.prepare()
            feedback.impactOccurred(intensity: 0.55)
        }
        backgroundColor = (accentColor ?? tintColor).withAlphaComponent(0.3)
        transform = CGAffineTransform(scaleX: 0.94, y: 0.94)
        onButtonChanged?(emulatorButton, true)
    }

    @objc private func releaseButton() {
        guard pressed else { return }
        pressed = false
        backgroundColor = UIColor.secondarySystemFill.withAlphaComponent(0.92)
        transform = .identity
        onButtonChanged?(emulatorButton, false)
    }
}
