# Building the SSH dependencies for Android

The R: device dials SSH BBSes through libssh, which needs OpenSSL. Neither is
shipped with Qt, so both are built once and picked up by `aspeqt.pro`.

Both live **outside** the repo, next to it, and the `.pro` finds them by relative
path. Override with `qmake ANDROID_OPENSSL_DIR=... LIBSSH_SRC=... LIBSSH_BUILD=...`
if your layout differs. If either is missing the build still succeeds — it just
warns and leaves SSH out (`HAVE_LIBSSH` undefined), so the desktop build is fine.

    ~/Projekty/Android/
      aspeqt/                  this repo
      android_openssl/         KDAB layout; holds the built OpenSSL
      libssh/                  libssh sources
      build-libssh-arm64/      libssh build tree

## OpenSSL

Use `../build-openssl-android.sh [version]` (defaults to the current LTS). It
downloads the release, applies KDAB's `ssl_3.patch` and installs into
`android_openssl/ssl_3/arm64-v8a/`, then verifies the result.

Three things that patch and script get right, and that a plain build does not:

* **`shlib_variant "_3"`** — output is `libcrypto_3.so` / `libssl_3.so` with
  matching SONAMEs. Android does not load versioned libraries, and the suffix
  keeps ours from clashing with the system OpenSSL.
* **`-Wl,-z,max-page-size=16384`** — Play rejects packages whose libraries are
  not 16 KB aligned.
* **`-D__ANDROID_API__=28`** — matches `ANDROID_MIN_SDK_VERSION`.

Keep the version current: Google Play's App Security Improvement programme flags
apps that bundle an OpenSSL with known CVEs, so prefer a supported LTS branch and
rebuild before a release.

## libssh

Built **static** on purpose: libssh sets a SOVERSION (`libssh.so.4.x`), which
Android will not load, and linking it into `libAspeQt.so` keeps the package to
one library instead of two.

    cmake -S ../libssh -B ../build-libssh-arm64 \
      -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-D_BSD_SOURCE -D_DEFAULT_SOURCE" \
      -DBUILD_SHARED_LIBS=OFF \
      -DWITH_EXAMPLES=OFF -DWITH_SERVER=OFF -DUNIT_TESTING=OFF \
      -DCLIENT_TESTING=OFF -DWITH_GSSAPI=OFF -DWITH_ZLIB=OFF -DWITH_PCAP=OFF \
      -DOPENSSL_INCLUDE_DIR=../android_openssl/ssl_3/include \
      -DOPENSSL_CRYPTO_LIBRARY=../android_openssl/ssl_3/arm64-v8a/libcrypto.so \
      -DOPENSSL_SSL_LIBRARY=../android_openssl/ssl_3/arm64-v8a/libssl.so
    cmake --build ../build-libssh-arm64 -j$(nproc)

`-D_BSD_SOURCE -D_DEFAULT_SOURCE` is required: Bionic hides `GLOB_TILDE` behind
`__USE_BSD`, and `src/config.c` fails to compile without it. Passing the macros
avoids patching libssh, so it stays trivial to upgrade.

Rebuild libssh after changing OpenSSL — it compiles against those headers.

## Verifying a build

`../build-release.sh` checks 16 KB alignment and uncompressed packaging of every
`.so`. To confirm the pieces landed:

    llvm-readelf -d libAspeQt_arm64-v8a.so | grep NEEDED   # libssl_3, libcrypto_3
    llvm-nm libAspeQt_arm64-v8a.so | grep ssh_connect      # libssh linked in
