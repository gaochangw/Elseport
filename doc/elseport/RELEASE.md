# Source publication and optional binary releases

The current deliverable is a public source repository with English and Chinese
README files. An App Store release, Developer ID identity and notarization are
not prerequisites for this source-only scope.

## Source publication

- [x] Elseport name and editable brand assets prepared.
- [x] README files distinguish the design prototype from the existing frontend.
- [x] Origin, modification notice and license entry points retained.
- [x] Build instructions and dependency notes prepared.
- [x] Private security contact designated in SECURITY.md.
- [x] Integration documentation states tested configurations and limitations.
- [ ] Review the exact publication diff and its source manifest.
- [ ] Exclude private experiments, test credentials, VM disks, installation
      media, internal evidence and local configuration.
- [ ] Confirm that no private platform-specific additions enter the public diff.
- [ ] Validate the selected source tree, then record the public revision.

Keep existing upstream platform support and license notices. Publish only the
reviewed changes; do not push a working directory containing private work as a
single unchecked patch. Report possible vulnerabilities privately before
publishing their details.

## Optional development packages

The local packaging helper makes a relocatable development bundle and signs it
ad hoc for local testing. Its dependency manifest and checksums describe that
particular bundle, not the entire source project. It keeps development settings
separate from an existing upstream installation.

Record the source revision and patch set, host and guest versions, dependency
versions, build options, packaging manifest and validation results. Exclude test
helpers and installation media. A successful local launch does not establish
compatibility on other machines.

## If public binary downloads are added later

Prepare the exact corresponding source and complete third-party license
inventory beside each binary. Define supported versions and maintenance scope.
Validate installation, relocation, launch, shutdown and guest integration on
clean machines. Evaluate the appropriate Apple signing and notarization process
for that distribution route. Those are future binary-release tasks, not blockers
for the current source repository.

Tests must cover sharing disabled, selected paths, malformed input,
cancellation and isolation between VMs. Test the real clipboard only after the
owner explicitly authorizes replacing its contents.
