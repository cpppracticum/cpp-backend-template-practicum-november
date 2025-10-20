name: PR Closer
on: [issues, pull_request]
jobs:
    pr_closer:
        if: github.repository_owner == 'cpppracticum'
        runs-on: ubuntu-latest
        steps:
        - name: Autoclose issues that did not follow issue template
        uses: roots/issue-closer@v1.1
        with:
            repo-token: ${{ secrets.GITHUB_TOKEN }}
            issue-close-message: "@${issue.user.login}. Issue был автоматически закрыт, так как этот репозиторий не принимает замечания. Передайте ваши замечания наставнику или куратору."
            pr-close-message: "@${pr.user.login}. PR был автоматически закрыт. Чтобы сдать задание на ревью, PR нужно открыть в своём репозитории. https://github.com/${pr.user.login}/cpp-backend-template-practicum-november/compare"
            issue-pattern: ".*PRACTICUMFB.*"
            pr-pattern: ".*PRACTICUMFB.*"
