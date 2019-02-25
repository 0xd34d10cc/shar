function upload-ctest-report()
{
    Param([Parameter(Mandatory=$True)][System.IO.FileInfo]$report)

    $dir = $PSScriptRoot
    $XSLFileName = "CTest2JUnit.xsl"
    $XSLFileInput = [System.IO.Path]::Combine($dir, $XSLFileName)

    $XSLInputElement = New-Object System.Xml.Xsl.XslCompiledTransform
    $XsltSettings = New-Object System.Xml.Xsl.XsltSettings($true, $false);
    $XmlUrlResolver = New-Object System.Xml.XmlUrlResolver
    $XSLInputElement.Load($XSLFileInput, $XsltSettings, $XmlUrlResolver)

    $reader = [System.Xml.XmlReader]::Create($report.FullName)
    $transformedPath = [System.IO.FileInfo]([System.IO.Path]::Combine($dir, 'tmp.xml'))
    $writer = [System.Xml.XmlTextWriter]::Create($transformedPath.FullName)

    $XSLInputElement.Transform($reader, $writer)

    $writer.Dispose();
    $reader.Dispose();

    $wc = New-Object System.Net.WebClient
    $wc.UploadFile("https://ci.appveyor.com/api/testresults/junit/$($env:APPVEYOR_JOB_ID)", $transformedPath.FullName)
}

foreach($file in $(ls Testing\*\Test.xml))
{
    upload-ctest-report $file
}