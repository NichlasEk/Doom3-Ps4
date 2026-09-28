# Launcher icon

`doom3-icon.svg` is original vector artwork for this port: Mars, metallic
DOOM lettering and an orange 3. `icon0.png` is the 512x512 opaque RGB version
used by the PS4 package. It contains no extracted retail artwork.

Regenerate the required PNG representation:

```sh
rsvg-convert assets/doom3-icon.svg -o assets/icon0.png
magick assets/icon0.png -alpha off -depth 8 PNG24:assets/icon0.png
```

The built-in image generator was attempted but returned moderation errors;
no AI-generated bitmap was used. The final icon was authored directly in SVG.
Attempted final prompt: "Create a square game launcher icon, opaque 512x512
composition. A beautifully detailed orange planet Mars against black space,
a subtle blue orbital glow, metallic silver bold title text exactly DOOM 3
large and centered across the planet. Clean dramatic science fiction graphic
design, excellent readability at small size, no other text, no figures,
no violence, no watermark, no frame or rounded corners. This is a custom
personal homebrew launcher icon."
