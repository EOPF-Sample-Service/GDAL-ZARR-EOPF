# Security Policy

## Supported Versions

We actively support the following versions with security updates:

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

We take the security of the GDAL EOPF-Zarr Plugin seriously. If you discover a security vulnerability, please follow these steps:

### 1. **Do NOT** create a public GitHub issue

Security vulnerabilities should be reported privately to protect users until a fix is available.

### 2. Report via GitHub Security Advisories

1. Go to the [Security tab](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/security) of this repository
2. Click "Report a vulnerability"
3. Fill out the security advisory form with:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if known)

### 3. Alternative: Email Report

If GitHub Security Advisories are not available, you can email the maintainers at:

- **Primary Contact**: [security@eopf-sample-service.org]
- Include "[SECURITY]" in the subject line

### 4. What to Include

When reporting a vulnerability, please include:

- **Description**: Clear description of the vulnerability
- **Location**: File/function/line where the issue exists
- **Impact**: What an attacker could achieve
- **Reproduction**: Step-by-step instructions to reproduce
- **Fix Suggestion**: If you have ideas for fixing (optional)
- **CVE Information**: If already assigned (optional)

## Response Timeline

- **Acknowledgment**: Within 48 hours
- **Initial Assessment**: Within 5 business days  
- **Status Updates**: Weekly until resolved
- **Fix Timeline**: Depends on severity
  - **Critical**: 7 days
  - **High**: 30 days
  - **Medium**: 90 days
  - **Low**: Next minor release

## Security Best Practices

When using the GDAL EOPF-Zarr Plugin:

### Input Validation
- Always validate file paths and URLs
- Use GDAL's virtual file system (VSI) for network access
- Implement proper error handling for malformed data

### Network Security
- Use HTTPS when accessing remote Zarr datasets
- Validate SSL certificates in production
- Consider network timeouts for remote data access

### Memory Safety
- The plugin handles large datasets - monitor memory usage
- Use GDAL's chunked reading for large files
- Implement proper cleanup in error paths

## Known Security Considerations

### 1. Remote File Access
- The plugin can access HTTP/HTTPS URLs
- Ensure network security policies are in place
- Consider firewall rules for outbound connections

### 2. Large File Handling
- Memory exhaustion possible with very large datasets
- Implement appropriate limits in production environments

### 3. Metadata Processing
- JSON metadata is parsed from Zarr files
- Malformed metadata could cause parsing issues
- Input validation is implemented but should be monitored

## Security Updates

Security updates will be:
- Released as patch versions (e.g., 1.0.1, 1.0.2)
- Documented in CHANGELOG.md with security advisory references
- Announced via GitHub releases with security labels
- Published to relevant security databases (CVE, GitHub Security Advisories)

## Contact

For general security questions (not vulnerabilities):
- **GitHub Discussions**: [Security category](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)
- **GitHub Issues**: Use the "security" label for non-sensitive questions

For urgent security matters:
- **GitHub Security Advisories**: [Report a vulnerability](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/security)

---

*This security policy is based on industry best practices and will be updated as needed.*
