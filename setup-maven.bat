c:
cd %USERPROFILE%
mkdir .m2
cd .m2
bitsadmin /transfer myDownLoadJob /download /priority normal "http://doc.canglaoshi.org/config/settings.xml" "%cd%/settings.xml"
echo Done!