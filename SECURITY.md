# Security reporting

Elseport is an independent, pre-release project. It has no supported public
binary releases yet. Its security contact does not imply that Oracle supports
this fork.

Do **not** report a possible vulnerability in a public issue, pull request,
patch, discussion or comment. Do not publish proof-of-concept material,
private guest data, credentials or logs containing secrets.

## Reporting now

For a problem in Elseport or its independent changes, email
[gaochangw@outlook.com](mailto:gaochangw@outlook.com) privately. Start with the
affected revision, a short description and how to arrange a safe exchange of
further evidence. Do not send credentials, private guest data or unnecessary
personal information. No encryption key is currently published here.

For a problem that may affect upstream VirtualBox code, follow
[Oracle's reporting process](https://www.oracle.com/corporate/security-practices/assurance/vulnerability/reporting.html).
The published contact for external reporters is
[secalert_us@oracle.com](mailto:secalert_us@oracle.com). Use that page's encryption
guidance for sensitive material. Oracle's process is for its products and does
not replace the independent project's responsibilities.

## Useful report contents

Include the affected revision, host and guest versions, relevant configuration,
expected and observed behaviour, and the smallest necessary reproduction.
Identify whether an unmodified upstream build is affected if safely checked.
Avoid unnecessary user data and redact secrets.

Coordinate disclosure before publishing a security-sensitive change. An issue
may involve this project and an upstream component. An unconfirmed crash is not
automatically a confirmed exploit; a successful test is not a full audit.

## Maintenance expectations

Before supporting a release, document supported versions, a private contact,
the security update policy and responsibility for monitoring dependencies.
The current development snapshot makes no response-time or support-period
commitment.

Non-security requests follow [CONTRIBUTING.md](CONTRIBUTING.md). Do not include
suspected vulnerability details in them.
