TEMPLATE = subdirs

SUBDIRS+= PCDM PCDM-conf

scripts=PCDMd
scripts.path=/usr/local/sbin
scripts.extra=cp PCDMd $(INSTALL_ROOT)/usr/local/sbin/PCDMd

rcd=rc.d/pcdm
rcd.path=/usr/local/etc/rc.d
rcd.extra=cp rc.d/pcdm $(INSTALL_ROOT)/usr/local/etc/rc.d/pcdm

cleanthemes.path=/usr/local/share/PCDM/themes
cleanthemes.extra=rm -r $(INSTALL_ROOT)/usr/local/share/PCDM/themes

theme=themes
theme.path=/usr/local/share/PCDM
theme.extra=cp -r themes $(INSTALL_ROOT)/usr/local/share/PCDM/.

xsession.files=xsessions/*.desktop
xsession.path=/usr/local/share/xsessions

INSTALLS += scripts rcd cleanthemes theme xsession
