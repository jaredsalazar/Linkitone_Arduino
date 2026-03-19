# LinkIt ONE Boards Manager packaging

Use the following scripts in order:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-linkit-one-modern-package.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build-linkit-one-boards-manager-package.ps1 -BaseUrl "https://your-host/path" -PackagerName "linkitone-community" -Maintainer "LinkIt ONE Community"
```

This produces a Boards Manager-ready folder in:

```text
dist\boards-manager
```

Contents:

- `package_<packager>_index.json`
- a platform archive
- a Windows ARM toolchain archive
- a Windows MediaTek uploader archive

## Publish flow

1. Upload every file from `dist\boards-manager` to a public web host.
2. Give users the URL to `package_<packager>_index.json`.
3. Users add that URL to Arduino IDE Boards Manager preferences.

## GitHub release workflow

1. Create a GitHub repository.
2. Create a release tag such as `v1.5.6-linkit-modern.1`.
3. Upload the generated `.zip` files from `dist\boards-manager` as release assets.
4. Rebuild the package with:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-linkit-one-boards-manager-package.ps1 `
  -BaseUrl "https://github.com/<user>/<repo>/releases/download/v1.5.6-linkit-modern.1" `
  -PackagerName "linkitone-community" `
  -Maintainer "LinkIt ONE Community" `
  -WebsiteUrl "https://github.com/<user>/<repo>"
```

5. Commit the generated `package_<packager>_index.json` from the repo root to your repository.
6. Share the Raw GitHub URL for that JSON file.

## Current limitations

- Windows only
- This is a legacy adapted package, not an upstream-supported board package
- Existing manual installs should be removed before switching to Boards Manager
