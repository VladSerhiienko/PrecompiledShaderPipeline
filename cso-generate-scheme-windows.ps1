
Remove-Item ".\include\generated" -Recurse -Force -ErrorAction Ignore
Remove-Item ".\include\mutable_generated" -Recurse -Force -ErrorAction Ignore
.\bin\Windows\flatc.exe -o .\include\generated -c .\schemes\cso.fbs
.\bin\Windows\flatc.exe -o .\include\mutable_generated -c .\schemes\cso.fbs --gen-mutable
