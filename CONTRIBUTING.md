# Contributing to Wireless Quiz Buzzer

First off, thank you for considering contributing to this project! 🎉

## Table of Contents
- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Process](#development-process)
- [Code Style Guidelines](#code-style-guidelines)
- [Commit Messages](#commit-messages)
- [Branch Naming](#branch-naming)

## Code of Conduct

This project adheres to a [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs
Before creating bug reports, please check existing issues. When creating a bug report, include:

- **Clear title and description**
- **Steps to reproduce**
- **Expected vs actual behavior**
- **Screenshots** (if applicable)
- **Hardware/software versions**

**Bug Report Template:**
```markdown
**Describe the bug**
A clear description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:
1. Go to '...'
2. Click on '...'
3. See error

**Expected behavior**
What you expected to happen.

**System Info:**
 - ESP32 Board: [e.g. DevKit v1]
 - Firmware Version: [e.g. 1.0.0]
 - Browser: [e.g. Chrome 120]
```

### Suggesting Features
Feature requests are welcome! Please provide:

- **Clear use case**
- **Expected behavior**
- **Possible implementation** (if you have ideas)
- **Alternatives considered**

### Pull Requests

1. **Fork the repo** and create your branch from `Main`
2. **Make your changes** with clear commits
3. **Test thoroughly** on actual hardware
4. **Update documentation** if needed
5. **Submit pull request** with description

## Development Process

### Setting Up Development Environment

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/wireless-quiz-buzzer.git

# Add upstream remote
git remote add upstream https://github.com/asifahamed-ece/wireless-quiz-buzzer.git

# Create feature branch
git checkout -b feature/amazing-feature

# Keep your branch updated
git fetch upstream
git rebase upstream/Main
```

### Testing Requirements

Before submitting, ensure:
- ✅ Master ESP32 boots correctly
- ✅ Slave ESP32 connects to master
- ✅ Dashboard loads without errors
- ✅ LISTEN → READY → ANSWERED flow works
- ✅ Audio feedback plays correctly
- ✅ Battery monitoring displays properly
- ✅ No console errors in browser

### Code Style Guidelines

#### C++ (ESP32 Firmware)
```cpp
// Use descriptive variable names
int connectedTeams = 0;  // Good

// Comment complex logic
if(voltage > 3.7) {
  zone = 2;  // Green: Healthy
}

// Use consistent formatting
void functionName() {
  if(condition) {
    // code
  }
}
```

#### JavaScript (Dashboard)
```javascript
// Use modern ES6+ syntax
const updateDisplay = (data) => {
  // Arrow functions preferred
};

// Clear variable names
let currentPhase = "LISTEN";  // Good
```

## Commit Messages

### Format

One line, imperative mood, 72 characters or fewer:

```
docs: correct pin table against the firmware
fix: drop ignored timestamp from the receive path
build: re-encode platformio.ini as UTF-8
```

Keep the body out of the commit message. The diff already lists the changed
files, and the reason belongs in the pull request description or an issue
reference.

### Prefixes

Use a prefix where it helps scanning. Omit it for trivial changes.

| Prefix | Use for |
|--------|---------|
| `feat:` | New capability |
| `fix:` | Bug fix |
| `docs:` | Documentation only |
| `refactor:` | Restructuring with no behaviour change |
| `build:` | Build system or dependencies |
| `chore:` | Maintenance that fits nowhere else |
| `test:` | Adding or changing tests |

### Examples

```
docs: correct battery thresholds in API reference
fix: master MAC banner was one character short
chore: remove resolved TODO markers from team firmware
```

## Branch Naming

- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation
- `refactor/` - Code improvements

## Questions?

Feel free to:
- Open an issue with `question` label
- Email: asifahamed670@gmail.com

---

**Thank you for contributing! 🎉**
