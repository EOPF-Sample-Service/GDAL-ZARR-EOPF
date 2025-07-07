---
name: Bug Report
about: Report a bug in the EOPF Zarr GDAL plugin
title: 'Bug: [Brief description]'
labels: bug
assignees: ''

---

## Bug Description
**Brief summary**: 
[What went wrong?]

**Expected behavior**: 
[What should have happened?]

**Actual behavior**: 
[What actually happened?]

## Environment
**Operating System**: (e.g., Windows 11, macOS 14.2, Ubuntu 22.04)
**Architecture**: (e.g., x86_64, arm64)
**GDAL Version**: 
```bash
gdalinfo --version
```

**Plugin Status**:
```bash
gdalinfo --formats | grep EOPFZARR
```

## Steps to Reproduce
1. [First step]
2. [Second step]
3. [Third step]
4. [etc.]

## Error Output
```
[Copy and paste the full error message/output here]
```

## Additional Context
**Dataset information** (if relevant):
- Dataset type: [local file, remote URL, etc.]
- Dataset size: [approximate size]
- Dataset format: [Zarr structure details if known]

**Workarounds found** (if any):
[Describe any temporary solutions you found]

**Screenshots** (if helpful):
[Attach screenshots if they help explain the issue]

## Severity
- [ ] **Critical** - Plugin completely unusable
- [ ] **High** - Major functionality broken
- [ ] **Medium** - Some features don't work
- [ ] **Low** - Minor issue or cosmetic problem

## Test Data
- [ ] Can reproduce with publicly available data
- [ ] Only occurs with specific/private dataset
- [ ] Occurs with any dataset
- [ ] No dataset needed to reproduce

If you can share test data that reproduces the issue, please provide:
- Download link or instructions
- Specific commands that fail

---

**Note**: For Windows users, "Error 126: Can't load requested DLL" is a known cosmetic issue that doesn't prevent functionality. Please only report if the plugin actually fails to work.
