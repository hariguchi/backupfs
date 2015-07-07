#!/bin/sh
#
# /etc/cron.daily/backup.sh: daily backup script
#

PATH=${PATH}:/usr/local/bin
export PATH

rootDir=/backup/hosts
hanaDir=${rootDir}/hana
pochiDir=${rootDir}/pochi
date=`date +%Y-%m%d`

# Clean up core files first
find /home -follow -type f \( -regex '.*core\.[0-9]+' -o -name core \) \
                                     -exec rm '{}' \; >/dev/null 2>/dev/null


date >$rootDir/log-$date
df  >>$rootDir/log-$date

backupfs /home              $hanaDir   >$hanaDir/log  2>$hanaDir/error-log
backupfs pochi:/etc         $pochiDir  >$pochiDir/log 2>$pochiDir/error-log
backupfs pochi:/root        $pochiDir >>$pochiDir/log 2>>$pochiDir/error-log
backupfs pochi:/var         $pochiDir >>$pochiDir/log 2>>$pochiDir/error-log
backupfs pochi:/home/cvs    $pochiDir >>$pochiDir/log 2>>$pochiDir/error-log
backupfs pochi:/home/htdocs $pochiDir >>$pochiDir/log 2>>$pochiDir/error-log
backupfs pochi:/home/yoichi $pochiDir >>$pochiDir/log 2>>$pochiDir/error-log

df   >>$rootDir/log-$date
date >>$rootDir/log-$date

#growisofs -Z /dev/dvd -R -J -quiet /backup/hosts/pochi/`date +%Y/%m`
