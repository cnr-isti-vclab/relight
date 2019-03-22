echo off
cls
for %%F in (plane_*.jpg) do (
	if exist %%~nF.dzi (
		del "%%~nF.dzi"
		del /s /q "%%~nF_files" > nul
		rmdir /s /q "%%~nF_files"
	)
	vips dzsave %%~nF.jpg %%~nF --tile-size 256 --depth onetile --suffix .jpg[Q=95]
)
