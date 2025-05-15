@echo off
git add .
git commit --amend -m "Auto-sync" || exit /b 0  REM Exit if nothing to commit
git push --force-with-lease

REM
