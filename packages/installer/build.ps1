$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

if (-not (Get-Command java -ErrorAction SilentlyContinue)) {
  throw 'Java 21+ is required.'
}
if (-not (Get-Command mvn -ErrorAction SilentlyContinue)) {
  throw 'Maven is required to build the JavaFX installer.'
}

java -version
mvn -B clean package
mvn -B javafx:run
