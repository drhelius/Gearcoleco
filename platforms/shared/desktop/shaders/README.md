# Gearcoleco Shaders

Gearcoleco desktop shader presets use `.gshader` files. A preset is an INI-style file that describes one or more GLSL fragment shader passes. Presets placed in this `shaders` directory, up to 64, are discovered once when the desktop app starts and appear in the `Video > Shader` menu after `Pixel Perfect`. Shader presets are not applied while the debugger is enabled.

GLSL files are normal fragment shaders. Gearcoleco supplies a fullscreen vertex shader and prepends the correct GLSL version, so shader files should not include a `#version` line.

## Minimal Preset

```ini
[Preset]
Name=Scanlines Bilinear
Passes=1

[Pass0]
Path=scanlines.glsl
ScaleType=Viewport
Filter=Linear

[Parameters]
ScanlineIntensity=0.10

[Parameter.ScanlineIntensity]
Label=Intensity
Min=0.0
Max=1.0
Step=0.01
```

## Preset Sections

`[Preset]`

- `Name`: display name shown in the menu. If omitted, the filename without its extension is used.
- `Group`: optional submenu name. Presets with the same group are listed together in that submenu.
- `Passes`: number of passes, from 1 to 32.
- `Variants`: optional number of `[VariantN]` sections, up to 16.
- `DefaultVariant`: variant applied when the preset is loaded for the first time. Defaults to `0`.

`[PassN]`

- `Path`: GLSL fragment shader path. Relative paths are resolved from the `.gshader` file directory first, then from the `shaders` resource directory.
- `Alias`: optional name for the pass output. Later passes can sample it with that name. See Shader Inputs.
- `Filter`: `Nearest` by default. Use `Linear` or `Bilinear` for bilinear sampling on that pass input/output texture.
- `Feedback`: `true` when the pass needs the previous frame through `PassFeedback0`.
- `History`: `true` when the pass needs previous inputs to the same pass through `SourceHistory0` through `SourceHistory7`.
- `ScaleType`: sets both axes. Defaults to `Viewport`. Values are:
  - `Source` (or `Original`): relative to the original emulator frame, at native core resolution.
  - `Previous`: relative to the previous pass output. This is RetroArch's `source` scale type.
  - `Viewport` (or `FinalViewport`): relative to the final display viewport.
  - `Absolute`: fixed size from `AbsoluteWidth` and `AbsoluteHeight`.
- `ScaleTypeX`, `ScaleTypeY`: optional per-axis scale type overrides.
- `Scale`: output scale for both axes. Defaults to `1.0`.
- `ScaleX`, `ScaleY`: optional per-axis scale overrides.
- `AbsoluteWidth`, `AbsoluteHeight`: used with `ScaleType=Absolute`.
- `FloatFramebuffer`: `true` when the pass output needs signed or high-range intermediate values. Uses a floating-point render target.

The final pass is what Gearcoleco displays. Non-final passes render to intermediate textures. Only the final pass needs
`Viewport`; passes that work at core resolution should use `Source` or `Previous` to avoid rendering at display size.

`[Parameters]`

Each entry creates a float uniform with the same name:

```ini
[Parameters]
Amount=0.5
```

Optional metadata for sliders:

```ini
[Parameter.Amount]
Label=Amount
Min=0.0
Max=1.0
Step=0.01
```

`Label` defaults to the parameter name. `Min`, `Max` and `Step` default to `0.0`, `1.0` and `0.01`, and the `[Parameters]`
value is clamped to the `Min` to `Max` range.

A parameter with `Min=0.0`, `Max=1.0` and a `Step` of `1.0` or higher is displayed as a checkbox. Other parameters with
a `Step` of `1.0` or higher are displayed as integer sliders.

Optional `Group` puts the parameter in a collapsible section of the Shader Parameters window, which opens from
`Video > Shader > Parameters...`:

```ini
[Parameter.Amount]
Label=Amount
Group=Glow
```

A preset can expose up to 256 parameters.

`[VariantN]`

A variant is a named set of parameter values. Selecting it resets every parameter to its `[Parameters]` value and then
applies the variant values. Variants are listed as submenus of the preset in `Video > Shader` and in the Shader
Parameters window. `Restore Defaults` restores the selected variant.

```ini
[Preset]
Name=My CRT
Variants=2

[Variant0]
Name=Slot Mask

[Variant1]
Name=Aperture Grille
MaskType=2.0
```

## Shader Inputs

Available samplers:

- `uniform sampler2D Source`: current pass input.
- `uniform sampler2D Original`: original emulator frame.
- `uniform sampler2D PassFeedback0`: previous final output, for feedback presets.
- `uniform sampler2D SourceHistory0` through `SourceHistory7`: previous inputs to the same pass, newest first, for history presets.
- `uniform sampler2D PassOutputN`: output of any previous pass, where `PassOutput0` is pass 0 output, `PassOutput1` is pass 1 output, and so on.
- `uniform sampler2D <Alias>`: output of a previous pass with that `Alias`.
- `uniform sampler2D <Alias>Feedback`: previous frame output of the non-final pass with that `Alias`. A pass can sample its own feedback.
- `uniform sampler2D OriginalIndex`: one texel per `Original` texel, with the same size and layout, holding the VDP color
  index of that texel in the red channel. Use the same coordinates as `Original` and read the index with
  `int(texture(OriginalIndex, uv).r * 255.0 + 0.5)`. It is always sampled without filtering. The TMS9918A produces the
  color numbers `1` to `15`: `1` black, `2` medium green, `3` light green, `4` dark blue, `5` light blue, `6` dark red,
  `7` cyan, `8` medium red, `9` light red, `10` dark yellow, `11` light yellow, `12` dark green, `13` magenta, `14` gray
  and `15` white. Transparent color `0` is replaced by the backdrop color, which is black (`1`) when register 7
  selects `0`. The F18A produces palette entries `0` to `63`. Overscan border texels hold the backdrop color index, and
  all texels are `0` while the Adam key mapper is open. The index texture follows the last
  emulated frame, so it does not match `Original` while rewinding or right after loading a state, until the emulator
  produces a new frame.

A pass can use up to 16 samplers.

Available size uniforms are `vec4(width, height, 1.0 / width, 1.0 / height)`:

- `SourceSize`: current pass input size.
- `OriginalSize`: original emulator frame size.
- `OutputSize`: current pass output size.
- `FinalViewportSize`: final display viewport size.
- `PassFeedback0Size`: feedback texture size.
- `SourceHistorySize`: source history texture size.
- `PassOutputSizeN` (or `PassOutputNSize`): previous pass output texture sizes.
- `<Alias>Size`: output size of the pass with that `Alias`.

Other uniforms:

- `FrameCount`: increments once per new emulated frame. It starts at `0` when a preset is loaded.
- `FrameDirection`: `1` normally, `-1` while rewinding.
- `SourceHistoryCount`: number of valid source history textures, from `0` to `8`.
- `OriginalAspect`: width divided by height of `Original`, such as `256 / 192`. It does not follow
  `Video > Aspect Ratio`.
- `FramebufferScale`: display pixel density, as the rounded ratio between framebuffer pixels and window points, at
  least `1.0`. Use it to keep masks and grids at a fixed physical size on high-density displays.
- `BackgroundColor`: configured normal desktop background color as `vec4(r, g, b, 1)`.

ColecoVision video uniforms, updated every frame:

- `float OriginalFPS`: emulated frame rate in Hz, about `59.92` on NTSC and `50.16` on PAL.
- `float PixelClock`: rate of the `Original` texels in MHz. It is `5.369`, the TMS9918A dot clock of 1.5 times the
  3.579545 MHz CPU clock, with one texel per VDP pixel: in every TMS9918A mode, with or without overscan, and in the
  F18A 256 and 240 pixel wide modes. It is `10.739` when `Original` is more than 320 texels wide, which are the F18A 480
  pixel 80-column text and 512 pixel modes and the Adam key mapper screen. PAL machines report the same values
  because the core uses the same 3.579545 MHz clock for both regions.
- `float LineRate`: horizontal scan rate in kHz, `15.700`: 228 CPU clocks per line.
- `float SourceLines`: number of scanlines in `Original`. It always equals `OriginalSize.y` here. Scanline shaders use
  it instead of `OriginalSize.y` so they also work in emulators that double lines.
- `vec4 ChromaPhase`: NTSC color subcarrier timing, for composite shaders. The TMS9918A takes its color subcarrier
  from the 3.579545 MHz CPU clock, so a line holds exactly 228 cycles and the pattern is the same on every row and
  every frame. `x` is the subcarrier phase at the left edge of `Original` row `0`, in cycles from `0.0` to `1.0`: `0.0`
  without horizontal overscan and `0.667` with `Video > Overscan > Full (284 width)` or `Full (320 width)`, whose extra
  border columns start before the picture. `y` is cycles per `Original` texel, `2/3`. `z` is cycles per row, `228`.
  `w` is the fractional cycles added every frame, `0.0`. The phase at `Original` position `(px, row)` is
  `fract(x + px * y + row * z)`, which repeats every 3 texels. PAL machines, the F18A and the Adam key mapper report
  `vec4(0.0)`.
- `vec4 ActiveRegion`: picture area inside `Original` as `vec4(x, y, width, height)` in texels. On the TMS9918A it is
  the `256x192` picture without the overscan border: `x` is `0` for `Disabled` and `Top+Bottom`, `14` for
  `Full (284 width)` and `32` for `Full (320 width)`, and `y` is `0` with overscan disabled, otherwise `24` on NTSC or
  `48` on PAL. In Text mode the picture is the `240` pixel window with 6 backdrop pixels on its left and 10 on its
  right, so `x` is 6 higher and `width` is `240`. It is the whole frame on the F18A and with the Adam key mapper.
- `vec4 BorderColor`: backdrop color from the low nibble of VDP register 7 as `vec4(r, g, b, 1)`, taken from the
  palette in use (`Video > Palette`). A backdrop of `0` is shown as black, like the picture does. On the F18A it comes
  from the F18A palette, including the palette select bits of register 24 in its enhanced modes. `vec4(0.0)` while the
  Adam key mapper is open and when no content is loaded.
- `int SystemType`: `0` TMS9918A, `1` F18A. Follows `Video > Video Chip`, where `Auto` uses the F18A only for games
  that require it.
- `int VideoMode`: VDP display mode from the mode bits of registers 0 and 1: bit 0 is Text, bit 1 is Graphics II and
  bit 2 is Multicolor. `0` Graphics I, `1` Text, `2` Graphics II, `4` Multicolor. The undocumented combinations are
  `3` Text with Graphics II, `5` and `7` Text with Multicolor, and `6` Graphics II with Multicolor. The F18A reports the
  same value. Its extra modes are not separate values: `OriginalSize.x` is `480` in 80-column text and `512` in the
  wide modes.

`FrameCount`, `SourceHistory`, `PassFeedback0` and `<Alias>Feedback` only advance when the emulator produces a new
frame, or when rewinding steps back one. Redraws while paused, after a window resize or after a parameter change render
the same frame again with the same state.

Any parameter declared in `[Parameters]` is also available as a `float` uniform.

## Minimal GLSL Shader

```glsl
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;

void main()
{
    FragColor = texture(Source, vTexCoord);
}
```

## Feedback Example

```ini
[Preset]
Name=Feedback Blend
Passes=1

[Pass0]
Path=feedback_blend.glsl
ScaleType=Viewport
Filter=Nearest
Feedback=true

[Parameters]
FeedbackAmount=0.5
```

A feedback shader can sample the previous final frame. Save this one as `feedback_blend.glsl` next to the preset:

```glsl
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform sampler2D PassFeedback0;
uniform float FeedbackAmount;

void main()
{
    vec3 current = texture(Source, vTexCoord).rgb;
    vec3 previous = texture(PassFeedback0, vTexCoord).rgb;
    FragColor = vec4(mix(current, previous, FeedbackAmount), 1.0);
}
```
