pkgname=tc956x-dkms
pkgver=2026.01.07.r1.g1b5474b
pkgrel=1
pkgdesc="Toshiba TC956X (Qualcomm QPS615) PCIe Ethernet Driver DKMS"
arch=(any)
url="https://github.com/strongtz/tc956x-dkms"
license=('GPL-2.0-or-later')
depends=(dkms)
makedepends=(rsync)
options=(!strip !debug)
source=(PKGBUILD)
sha256sums=(SKIP)

pkgver() {
	realdir=$(dirname $(realpath $srcdir/PKGBUILD))
	cd ${realdir}
	ver="$(git describe --tags 2>/dev/null | sed 's/^[vV]//;s/\([^-]*-g\)/r\1/;s/-/./g')"
	if [ -z "$ver" ]; then
		cnt="$(git rev-list --count HEAD)"
		rev="$(git rev-parse --short HEAD)"
		printf "r%s.%s" "$cnt" "$rev"
	else
		printf "%s" "$ver"
	fi
}

package() {
	realdir=$(dirname $(realpath $srcdir/PKGBUILD))
	rsync --verbose --recursive --mkpath --exclude-from="$realdir/.gitignore" \
		"$realdir/dkms.conf" \
		"$realdir/drivers" \
		"$pkgdir/usr/src/$pkgname-$pkgver/"
	rsync --verbose --recursive --mkpath --exclude-from="$realdir/.gitignore" \
		"$realdir/hwdb/" \
		"$pkgdir/usr/lib/udev/hwdb.d/"
	sed -i \
		-e "s,^PACKAGE_VERSION=\".*\"$,PACKAGE_VERSION=\"$pkgver\",g" \
		-e "s,^PACKAGE_NAME=\".*\"$,PACKAGE_NAME=\"$pkgname\",g" \
		"$pkgdir/usr/src/$pkgname-$pkgver/dkms.conf"
}
