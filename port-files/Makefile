# Created by: Joe Maloney <jmaloney@ixsystems.com>
# $FreeBSD$

PORTNAME=	pcdm
PORTVERSION=	%%CHGVERSION%%
CATEGORIES=	x11

MAINTAINER=	jmaloney@ixsystems.com
COMMENT=	QT5 based display manager for FreeBSD

LICENSE=	BSD2CLAUSE

RUN_DEPENDS=    xorg-minimal>=0:${PORTSDIR}/x11/xorg-minimal

CONFLICTS=      pcbsd-utils-qt*

USE_QT5=        core gui network svg multimedia imageformats \
                buildtools_build x11extras concurrent
USES=           qmake

USE_GITHUB=	yes
GH_ACCOUNT=	trueos
GH_PROJECT=	pcdm
GH_TAGNAME=	%%GHTAG%%

.include <bsd.port.mk>
