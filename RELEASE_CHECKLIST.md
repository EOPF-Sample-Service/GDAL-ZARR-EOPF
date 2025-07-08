# Release Checklist for Test Users

Use this checklist before distributing the plugin to test users.

## Pre-Release Testing

### Build System Check
- [ ] GitHub Actions workflow completes successfully
- [ ] All platform builds (Windows, macOS x86_64, macOS arm64, Linux) complete
- [ ] Universal macOS binary is created successfully
- [ ] No build errors or critical warnings

### Artifact Verification
- [ ] Windows: `gdal_EOPFZarr.dll` is present and correct size
- [ ] macOS Intel: `gdal_EOPFZarr.dylib` (x86_64) is present
- [ ] macOS Apple Silicon: `gdal_EOPFZarr.dylib` (arm64) is present  
- [ ] macOS Universal: `gdal_EOPFZarr.dylib` (universal) is present
- [ ] Linux: `gdal_EOPFZarr.so` is present and correct size

### Installation Scripts
- [ ] `install-windows.bat` is generated correctly
- [ ] `install-macos.sh` is generated correctly
- [ ] `install-linux.sh` is generated correctly
- [ ] Scripts have correct permissions and syntax

### Internal Testing
- [ ] At least one team member has tested on Windows
- [ ] At least one team member has tested on macOS (Intel or Apple Silicon)
- [ ] At least one team member has tested on Linux
- [ ] Basic functionality confirmed on all tested platforms

## Documentation Check

### User Documentation
- [ ] `GETTING_STARTED.md` is up to date
- [ ] `TEST_CHECKLIST.md` is available and clear
- [ ] `README.md` includes test user section
- [ ] Installation instructions are platform-specific and accurate

### Issue Templates
- [ ] Test user report template is available
- [ ] Bug report template is available
- [ ] Templates are clear and helpful

### Known Issues Documentation
- [ ] Windows Error 126 issue is documented as harmless
- [ ] macOS architecture requirements are clearly stated
- [ ] Any platform-specific quirks are documented

## Release Package

### Package Contents
- [ ] Plugin binaries for all platforms
- [ ] Installation scripts for all platforms
- [ ] `GETTING_STARTED.md`
- [ ] `TEST_CHECKLIST.md`
- [ ] `README.md` (or link to it)
- [ ] `BUILD_INFO.json` with build metadata

### Package Structure
```
eopf-zarr-gdal-plugin-vX.X.X/
├── windows/
│   └── gdal_EOPFZarr.dll
├── macos/
│   ├── x86_64/gdal_EOPFZarr.dylib
│   ├── arm64/gdal_EOPFZarr.dylib
│   └── universal/gdal_EOPFZarr.dylib
├── linux/
│   └── gdal_EOPFZarr.so
├── install-windows.bat
├── install-macos.sh
├── install-linux.sh
├── GETTING_STARTED.md
├── TEST_CHECKLIST.md
└── BUILD_INFO.json
```

- [ ] Directory structure matches above
- [ ] All files are present
- [ ] File permissions are correct

## Release Notes

### User-Facing Release Notes
- [ ] Version number is correct
- [ ] New features are highlighted
- [ ] Known issues are mentioned
- [ ] Platform compatibility is stated
- [ ] Installation instructions are included

### Technical Release Notes
- [ ] Build environment details
- [ ] GDAL version compatibility
- [ ] Dependencies are listed
- [ ] Breaking changes (if any) are noted

## Communication

### GitHub Release
- [ ] Release is tagged with correct version
- [ ] Release notes are complete and user-friendly
- [ ] Artifacts are attached to the release
- [ ] Release is marked as pre-release if appropriate

### Test User Communication
- [ ] Test users are notified of new release
- [ ] Clear instructions on how to download and test
- [ ] Feedback collection process is explained
- [ ] Timeline expectations are set

## Expected Test User Experience

### Successful Installation Should Result In:
- [ ] Plugin installs without fatal errors
- [ ] Plugin appears in `gdalinfo --formats`
- [ ] Plugin can open test datasets (if available)
- [ ] Plugin works in QGIS and other GDAL applications

### Acceptable Issues:
- [ ] Windows Error 126 (documented as harmless)
- [ ] Minor warning messages that don't prevent functionality
- [ ] Performance quirks that don't break basic operations

### Unacceptable Issues:
- [ ] Plugin fails to install
- [ ] Plugin doesn't appear in GDAL formats
- [ ] Plugin crashes GDAL or applications
- [ ] Plugin completely fails to open any datasets

## Post-Release Monitoring

### Issue Tracking
- [ ] Monitor GitHub issues for test user reports
- [ ] Respond to test user questions promptly
- [ ] Categorize issues by severity and platform
- [ ] Update documentation based on feedback

### Success Metrics
- [ ] Track number of successful installations
- [ ] Track platforms where plugin works
- [ ] Track major blocking issues
- [ ] Track user satisfaction/feedback

## Decision Points

### Release Readiness
- [ ] **Go/No-Go**: Are the critical issues fixed?
- [ ] **Platform Coverage**: Do we have working binaries for major platforms?
- [ ] **Documentation**: Can users successfully install and test?

### Feedback Threshold
- [ ] **Minimum**: At least X successful test reports per platform
- [ ] **Diversity**: Test reports from different GDAL versions
- [ ] **Issues**: No critical/blocking issues remain unresolved

---

**Release Manager**: ___________  
**Release Date**: ___________  
**Version**: ___________  
**Test Period**: ___________ to ___________
