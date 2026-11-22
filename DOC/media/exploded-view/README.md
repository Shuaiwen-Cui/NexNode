# Exploded-view renders

Master renders of the 3D exploded-view loop (16 s, seamless), for slides and videos.
Not published by MkDocs — the site uses the lighter copies in `docs/static/liftnode-viewer/media/`.

| File | Use |
|---|---|
| `*-exploded-zh-dark.mp4` / `*-en-*.mp4` | 1920×1080 @ 60 fps, H.264 — insert into PowerPoint (enable loop) |
| `*-exploded-*.gif` | 960 px @ 20 fps fallback where video is not allowed |

Regenerate (PowerShell, Chrome or Edge installed):

```powershell
cd DOC\tools\viewer-capture; npm install
$env:BROWSER_PATH = "C:\Program Files\Google\Chrome\Application\chrome.exe"
node render.mjs --lang zh --brand NEXNODE; node render.mjs --lang en --brand NEXNODE      # this repo: add --brand NEXNODE
```

Then refresh the homepage loops (1080p30, ~3.5 MB):

```powershell
$ff = "node_modules\ffmpeg-static\ffmpeg.exe"
foreach ($l in "zh","en") { & $ff -y -i "..\..\media\exploded-view\nexnode-exploded-$l-dark.mp4" -vf fps=30 -c:v libx264 -preset slow -crf 23 -pix_fmt yuv420p -movflags +faststart -an "..\..\docs\static\liftnode-viewer\media\hero-$l.mp4" }
```
