# GitHub — BOOM v0.5

As of 2026-11-16, the connected GitHub search returned no existing BOOM repository. Repository creation, branch creation and source upload have **not** been performed on the user's account.

Recommended new repository: `BSM1973/Liberty-BOOM`.

1. Create repository with the GitHub prefilled form: https://github.com/new?owner=BSM1973&name=Liberty-BOOM&visibility=private . Select **Add README** to initialize its default branch. Press **Create repository**.
2. Open its branches page: https://github.com/BSM1973/Liberty-BOOM/branches ; choose **New branch**, use `development/boom-v0.5` based on `main` and create it.
3. Add the supplied sources to that branch **with their original directory structure**, including `.github/workflows/build.yml` and `CMakeLists.txt`. Do not upload the outer ZIP as a single file if builds are expected.
4. GitHub Actions will trigger automatically on a push to this branch. Under **Actions**, select **BOOM - native plugins and DSP tests** and review failures / downloadable unsigned artifacts.

If an existing private BOOM repository already exists, use *its* exact repository URL in step 2; do not create a duplicate.
