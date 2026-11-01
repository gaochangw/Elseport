# Contributing to Elseport

This independent project is based on VirtualBox. Read [AGENTS.md](AGENTS.md),
the subsystem's local guidance and [the coding guide](doc/VBox-CodingGuidelines.cpp)
before changing code. Keep changes focused and test the relevant target.

## Before sharing a change

- Follow [SECURITY.md](SECURITY.md). Handle possible vulnerabilities privately.
- Discuss non-security work through the project's designated channel. The
  upstream issue tracker is not the independent project's support queue.
- Preserve existing copyright and license notices. New files must accurately
  describe origin and license; do not copy a holder's name from a template.
- Explain the problem, cause, fix, validation, untested configurations and API
  impact. Update relevant documentation when behaviour changes.
- Use a `Signed-off-by:` line with your real contribution identity when
  preparing a contribution for submission. It does not itself assign copyright
  or sign a separate agreement.

Independent contributions must be compatible with the applicable GPLv3 and
third-party licenses. Do not restrict recipients' existing rights. This project
does not imply that an Oracle Contributor Agreement covers independent work.

## Upstream contributions

Upstream development centers on SVN; the Git mirror may lag it. Changes intended
for upstream must follow its contribution process, which requires an
[Oracle Contributor Agreement](https://oca.opensource.oracle.com/) and the
appropriate sign-off. Verify the current requirements before submission.
An independent contribution is not automatically an upstream submission.

Do not create commits, branches, tags, pushes or public reports on another
person's behalf without explicit instruction. For future SVN submission of new
files, use `svn-ps.sh` / `svn-ps.cmd` to set the correct properties.
