set(CC_ALL_OPTIONS "-D_FORTIFY_SOURCE=2 -O2 -pipe -Werror -Wextra -Wcast-qual -Wall -Wdate-time -Wfloat-equal -Wshadow -Wformat=2 -Wno-stringop-overread -fsigned-char -fno-common -fno-strict-aliasing -fno-omit-frame-pointer -fPIC -fstack-protector-strong --param=ssp-buffer-size=4 -DHITLS_CRYPTO_NIST_ECC_ACCELERATE -DHITLS_CRYPTO_BN_COMBA -DHITLS_CRYPTO_AES_PRECALC_TABLES -DHITLS_AARCH64_PACIASP -DHITLS_CRYPTO_SM2_PRECOMPUTE_512K_TBL -DHITLS_AUTH -DHITLS_AUTH_PRIVPASS_TOKEN -DHITLS_BSL -DHITLS_BSL_ASN1 -DHITLS_BSL_BASE64 -DHITLS_BSL_BUFFER -DHITLS_BSL_ERR -DHITLS_BSL_HASH -DHITLS_BSL_INIT -DHITLS_BSL_LIST -DHITLS_BSL_LOG -DHITLS_BSL_OBJ -DHITLS_BSL_PARAMS -DHITLS_BSL_PEM -DHITLS_BSL_SAL -DHITLS_BSL_SAL_DL -DHITLS_BSL_SAL_FILE -DHITLS_BSL_SAL_LINUX -DHITLS_BSL_SAL_LOCK -DHITLS_BSL_SAL_MEM -DHITLS_BSL_SAL_NET -DHITLS_BSL_SAL_STR -DHITLS_BSL_SAL_THREAD -DHITLS_BSL_SAL_TIME -DHITLS_BSL_TLV -DHITLS_BSL_UIO -DHITLS_BSL_USRDATA -DHITLS_CRYPTO -DHITLS_CRYPTO_BN -DHITLS_CRYPTO_CIPHER -DHITLS_CRYPTO_CODECS -DHITLS_CRYPTO_CODECSKEY -DHITLS_CRYPTO_DRBG -DHITLS_CRYPTO_EAL -DHITLS_CRYPTO_EALINIT -DHITLS_CRYPTO_ENTROPY -DHITLS_CRYPTO_HPKE -DHITLS_CRYPTO_KDF -DHITLS_CRYPTO_MAC -DHITLS_CRYPTO_MD -DHITLS_CRYPTO_MODES -DHITLS_CRYPTO_PKEY -DHITLS_CRYPTO_PROVIDER -DHITLS_PKI -DHITLS_PKI_INFO -DHITLS_PKI_PKCS12 -DHITLS_PKI_X509 -DHITLS_SIXTY_FOUR_BITS -DHITLS_TLS -DHITLS_TLS_CALLBACK -DHITLS_TLS_CONFIG -DHITLS_TLS_CONNECTION -DHITLS_TLS_FEATURE -DHITLS_TLS_HOST -DHITLS_TLS_MAINTAIN -DHITLS_TLS_PROTO -DHITLS_TLS_PROTO_VERSION -DHITLS_TLS_SUITE -DHITLS_TLS_SUITE_AUTH -DHITLS_TLS_SUITE_CIPHER -DHITLS_TLS_SUITE_KX")

set(SHARED_LNK_FLAGS  -shared -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now)

set(PIE_EXE_LNK_FLAGS  -shared -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now)

project(hitls_bsl C)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_C_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_ASM_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENHITLS_VERSION_S='\"openHiTLS 0.2.0 15 May 2025\"' -DOPENHITLS_VERSION_I=2097152 -D__FILENAME__='\"$(notdir $(subst .o,,$@))\"'")
include_directories(
    include/bsl
    bsl/err/include
    config/macro_config
    bsl/log/include
    bsl/include
)

# Add module asn1 
add_library(asn1-objs OBJECT)
target_include_directories(asn1-objs PRIVATE
    platform/Secure_C/include
    bsl/asn1/include
    bsl/sal/include
)
target_sources(asn1-objs PRIVATE
    bsl/asn1/src/bsl_asn1.c
    bsl/asn1/src/bsl_asn1_print.c
)

# Add module base64 
add_library(base64-objs OBJECT)
target_include_directories(base64-objs PRIVATE
    bsl/base64/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(base64-objs PRIVATE bsl/base64/src/bsl_base64.c)

# Add module buffer 
add_library(buffer-objs OBJECT)
target_include_directories(buffer-objs PRIVATE
    platform/Secure_C/include
    bsl/buffer/include
    bsl/sal/include
)
target_sources(buffer-objs PRIVATE bsl/buffer/src/bsl_buffer.c)

# Add module err 
add_library(err-objs OBJECT)
target_include_directories(err-objs PRIVATE
    platform/Secure_C/include
    bsl/err/include
    bsl/sal/include
)
target_sources(err-objs PRIVATE
    bsl/err/src/avl.c
    bsl/err/src/err.c
)

# Add module hash 
add_library(hash-objs OBJECT)
target_include_directories(hash-objs PRIVATE
    platform/Secure_C/include
    bsl/hash/include
)
target_sources(hash-objs PRIVATE
    bsl/hash/src/bsl_hash.c
    bsl/hash/src/bsl_hash_list.c
    bsl/hash/src/hash_local.c
    bsl/hash/src/list_base.c
)

# Add module init 
add_library(init-objs OBJECT)
target_include_directories(init-objs PRIVATE bsl/obj/include)
target_sources(init-objs PRIVATE bsl/init/bsl_init.c)

# Add module list 
add_library(list-objs OBJECT)
target_include_directories(list-objs PRIVATE
    platform/Secure_C/include
    bsl/list/include
    bsl/sal/include
)
target_sources(list-objs PRIVATE
    bsl/list/src/bsl_list.c
    bsl/list/src/bsl_list_ex.c
    bsl/list/src/bsl_list_internal.c
)

# Add module log 
add_library(log-objs OBJECT)
target_include_directories(log-objs PRIVATE
    platform/Secure_C/include
    bsl/log/include
)
target_sources(log-objs PRIVATE bsl/log/src/log.c)

# Add module obj 
add_library(obj-objs OBJECT)
target_include_directories(obj-objs PRIVATE
    platform/Secure_C/include
    bsl/obj/include
    bsl/hash/include
    bsl/sal/include
)
target_sources(obj-objs PRIVATE
    bsl/obj/src/bsl_cid_op.c
    bsl/obj/src/bsl_obj.c
)

# Add module params 
add_library(params-objs OBJECT)
target_include_directories(params-objs PRIVATE
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(params-objs PRIVATE bsl/params/src/bsl_params.c)

# Add module pem 
add_library(pem-objs OBJECT)
target_include_directories(pem-objs PRIVATE
    bsl/base64/include
    platform/Secure_C/include
    bsl/pem/include
    bsl/sal/include
)
target_sources(pem-objs PRIVATE bsl/pem/src/bsl_pem.c)

# Add module sal 
add_library(sal-objs OBJECT)
target_include_directories(sal-objs PRIVATE
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(sal-objs PRIVATE
    bsl/sal/src/linux/linux_sal_dl.c
    bsl/sal/src/linux/linux_sal_file.c
    bsl/sal/src/linux/linux_sal_lockimpl.c
    bsl/sal/src/linux/linux_sal_mem.c
    bsl/sal/src/linux/linux_sal_net.c
    bsl/sal/src/linux/linux_time_impl.c
    bsl/sal/src/sal_atomic.c
    bsl/sal/src/sal_ctrl.c
    bsl/sal/src/sal_dl.c
    bsl/sal/src/sal_file.c
    bsl/sal/src/sal_mem.c
    bsl/sal/src/sal_net.c
    bsl/sal/src/sal_string.c
    bsl/sal/src/sal_threadlock.c
    bsl/sal/src/sal_time.c
)

# Add module tlv 
add_library(tlv-objs OBJECT)
target_include_directories(tlv-objs PRIVATE
    platform/Secure_C/include
    bsl/tlv/include
)
target_sources(tlv-objs PRIVATE bsl/tlv/src/tlv.c)

# Add module uio 
add_library(uio-objs OBJECT)
target_include_directories(uio-objs PRIVATE
    bsl/buffer/include
    platform/Secure_C/include
    bsl/uio/include
    bsl/sal/include
)
target_sources(uio-objs PRIVATE
    bsl/uio/src/uio_abstraction.c
    bsl/uio/src/uio_addr.c
    bsl/uio/src/uio_buffer.c
    bsl/uio/src/uio_mem.c
    bsl/uio/src/uio_sctp.c
    bsl/uio/src/uio_tcp.c
    bsl/uio/src/uio_udp.c
)

# Add module usrdata 
add_library(usrdata-objs OBJECT)
target_include_directories(usrdata-objs PRIVATE)
target_sources(usrdata-objs PRIVATE bsl/usrdata/src/usr_data.c)

add_library(hitls_bsl-shared SHARED
    $<TARGET_OBJECTS:asn1-objs>
    $<TARGET_OBJECTS:base64-objs>
    $<TARGET_OBJECTS:buffer-objs>
    $<TARGET_OBJECTS:err-objs>
    $<TARGET_OBJECTS:hash-objs>
    $<TARGET_OBJECTS:init-objs>
    $<TARGET_OBJECTS:list-objs>
    $<TARGET_OBJECTS:log-objs>
    $<TARGET_OBJECTS:obj-objs>
    $<TARGET_OBJECTS:params-objs>
    $<TARGET_OBJECTS:pem-objs>
    $<TARGET_OBJECTS:sal-objs>
    $<TARGET_OBJECTS:tlv-objs>
    $<TARGET_OBJECTS:uio-objs>
    $<TARGET_OBJECTS:usrdata-objs>
)
target_link_options(hitls_bsl-shared PRIVATE ${SHARED_LNK_FLAGS})
set_target_properties(hitls_bsl-shared PROPERTIES OUTPUT_NAME hitls_bsl)
install(TARGETS hitls_bsl-shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(hitls_bsl-shared dl boundscheck)
target_link_libraries(hitls_bsl-shared sctp boundscheck)

add_library(hitls_bsl-static STATIC
    $<TARGET_OBJECTS:asn1-objs>
    $<TARGET_OBJECTS:base64-objs>
    $<TARGET_OBJECTS:buffer-objs>
    $<TARGET_OBJECTS:err-objs>
    $<TARGET_OBJECTS:hash-objs>
    $<TARGET_OBJECTS:init-objs>
    $<TARGET_OBJECTS:list-objs>
    $<TARGET_OBJECTS:log-objs>
    $<TARGET_OBJECTS:obj-objs>
    $<TARGET_OBJECTS:params-objs>
    $<TARGET_OBJECTS:pem-objs>
    $<TARGET_OBJECTS:sal-objs>
    $<TARGET_OBJECTS:tlv-objs>
    $<TARGET_OBJECTS:uio-objs>
    $<TARGET_OBJECTS:usrdata-objs>
)
set_target_properties(hitls_bsl-static PROPERTIES OUTPUT_NAME hitls_bsl)
install(TARGETS hitls_bsl-static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_executable(hitls_bsl-object
    $<TARGET_OBJECTS:asn1-objs>
    $<TARGET_OBJECTS:base64-objs>
    $<TARGET_OBJECTS:buffer-objs>
    $<TARGET_OBJECTS:err-objs>
    $<TARGET_OBJECTS:hash-objs>
    $<TARGET_OBJECTS:init-objs>
    $<TARGET_OBJECTS:list-objs>
    $<TARGET_OBJECTS:log-objs>
    $<TARGET_OBJECTS:obj-objs>
    $<TARGET_OBJECTS:params-objs>
    $<TARGET_OBJECTS:pem-objs>
    $<TARGET_OBJECTS:sal-objs>
    $<TARGET_OBJECTS:tlv-objs>
    $<TARGET_OBJECTS:uio-objs>
    $<TARGET_OBJECTS:usrdata-objs>
)
target_link_options(hitls_bsl-object PRIVATE ${PIE_EXE_LNK_FLAGS})
set_target_properties(hitls_bsl-object PROPERTIES OUTPUT_NAME libhitls_bsl.o)
install(TARGETS hitls_bsl-object DESTINATION ${CMAKE_INSTALL_PREFIX}/obj)


project(hitls_crypto C ASM)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_C_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_ASM_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENHITLS_VERSION_S='\"openHiTLS 0.2.0 15 May 2025\"' -DOPENHITLS_VERSION_I=2097152 -D__FILENAME__='\"$(notdir $(subst .o,,$@))\"'")
include_directories(
    bsl/err/include
    include/crypto
    config/macro_config
    bsl/log/include
    crypto/include
)

# Add module bn 
add_library(bn-objs OBJECT)
target_include_directories(bn-objs PRIVATE
    platform/Secure_C/include
    crypto/bn/include
    bsl/sal/include
)
target_sources(bn-objs PRIVATE
    crypto/bn/src/bn_basic.c
    crypto/bn/src/bn_bincal.c
    crypto/bn/src/bn_comba.c
    crypto/bn/src/bn_const.c
    crypto/bn/src/bn_gcd.c
    crypto/bn/src/bn_lcm.c
    crypto/bn/src/bn_mont.c
    crypto/bn/src/bn_nistmod.c
    crypto/bn/src/bn_operation.c
    crypto/bn/src/bn_optimizer.c
    crypto/bn/src/bn_prime.c
    crypto/bn/src/bn_rand.c
    crypto/bn/src/bn_sqrt.c
    crypto/bn/src/bn_ucal.c
    crypto/bn/src/bn_utils.c
    crypto/bn/src/noasm_bn_bincal.c
    crypto/bn/src/noasm_bn_mont.c
)

# Add module util 
add_library(util-objs OBJECT)
target_include_directories(util-objs PRIVATE platform/Secure_C/include)
target_sources(util-objs PRIVATE
    crypto/util/crypt_util_algId.c
    crypto/util/crypt_util_mgf.c
    crypto/util/crypt_util_pkey.c
    crypto/util/crypt_util_rand.c
)

# Add module aes 
add_library(aes-objs OBJECT)
target_include_directories(aes-objs PRIVATE
    platform/Secure_C/include
    crypto/aes/include
)
target_sources(aes-objs PRIVATE
    crypto/aes/src/crypt_aes.c
    crypto/aes/src/crypt_aes_sbox.c
    crypto/aes/src/crypt_aes_setkey.c
    crypto/aes/src/crypt_aes_tbox.c
)

# Add module sm4 
add_library(sm4-objs OBJECT)
target_include_directories(sm4-objs PRIVATE
    platform/Secure_C/include
    crypto/sm4/include
    bsl/sal/include
)
target_sources(sm4-objs PRIVATE
    crypto/sm4/src/crypt_sm4.c
    crypto/sm4/src/crypt_sm4.c
    crypto/sm4/src/crypt_sm4_public.c
    crypto/sm4/src/sm4_key.c
    crypto/sm4/src/sm4_key.c
)

# Add module chacha20 
add_library(chacha20-objs OBJECT)
target_include_directories(chacha20-objs PRIVATE
    platform/Secure_C/include
    crypto/chacha20/include
    bsl/sal/include
)
target_sources(chacha20-objs PRIVATE
    crypto/chacha20/src/chacha20.c
    crypto/chacha20/src/chacha20block.c
)

# Add module codecs 
add_library(codecs-objs OBJECT)
target_include_directories(codecs-objs PRIVATE
    crypto/provider/include
    bsl/list/include
    bsl/err/include
    bsl/hash/include
    codecs/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(codecs-objs PRIVATE
    codecs/src/decode.c
    codecs/src/decode_chain.c
)

# Add module codecskey 
add_library(codecskey-objs OBJECT)
target_include_directories(codecskey-objs PRIVATE
    crypto/bn/include
    crypto/rsa/include
    crypto/curve25519/include
    crypto/ecdsa/include
    crypto/provider/include
    crypto/codecskey/include
    bsl/asn1/include
    crypto/eal/include
    bsl/obj/include
    bsl/pem/include
    crypto/sm2/include
    crypto/ecc/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(codecskey-objs PRIVATE
    crypto/codecskey/src/crypt_decode_der2key.c
    crypto/codecskey/src/crypt_decode_ecc.c
    crypto/codecskey/src/crypt_decode_epki2pki.c
    crypto/codecskey/src/crypt_decode_lowkey2pkey.c
    crypto/codecskey/src/crypt_decode_pem2der.c
    crypto/codecskey/src/crypt_decode_pkey.c
    crypto/codecskey/src/crypt_decode_rsa.c
    crypto/codecskey/src/crypt_encode_decode.c
    crypto/codecskey/src/crypt_encode_decode_local.c
    crypto/codecskey/src/crypt_encode_decode_utils.c
)

# Add module drbg 
add_library(drbg-objs OBJECT)
target_include_directories(drbg-objs PRIVATE
    crypto/drbg/include
    crypto/eal/src
    crypto/entropy/include
    crypto/ealinit/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(drbg-objs PRIVATE
    crypto/drbg/src/drbg.c
    crypto/drbg/src/drbg_ctr.c
    crypto/drbg/src/drbg_hash.c
    crypto/drbg/src/drbg_hmac.c
)

# Add module eal 
add_library(eal-objs OBJECT)
target_include_directories(eal-objs PRIVATE
    crypto/modes/include
    crypto/bn/include
    crypto/hybridkem/include
    crypto/drbg/include
    crypto/chacha20/include
    crypto/mlkem/include
    crypto/eal/include
    crypto/md5/include
    crypto/sha2/include
    crypto/hmac/include
    crypto/aes/include
    crypto/sm4/include
    crypto/dh/include
    crypto/hkdf/include
    crypto/sha3/include
    crypto/elgamal/include
    crypto/ecdh/include
    crypto/siphash/include
    crypto/gmac/include
    crypto/sm3/include
    crypto/pbkdf2/include
    crypto/slh_dsa/include
    crypto/encode/include
    crypto/provider/include
    crypto/ealinit/include
    crypto/kdf/include
    crypto/scrypt/include
    crypto/mldsa/include
    crypto/ecc/include
    crypto/dsa/include
    crypto/sm2/include
    crypto/rsa/include
    crypto/curve25519/include
    crypto/ecdsa/include
    crypto/entropy/include
    crypto/sha1/include
    crypto/paillier/include
    crypto/cmac/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(eal-objs PRIVATE
    crypto/eal/src/eal_cipher.c
    crypto/eal/src/eal_cipher_method.c
    crypto/eal/src/eal_common.c
    crypto/eal/src/eal_entropy.c
    crypto/eal/src/eal_entropyPool.c
    crypto/eal/src/eal_entropy_ecf.c
    crypto/eal/src/eal_kdf.c
    crypto/eal/src/eal_kdf_method.c
    crypto/eal/src/eal_keymgmt_util.c
    crypto/eal/src/eal_mac.c
    crypto/eal/src/eal_mac_method.c
    crypto/eal/src/eal_md.c
    crypto/eal/src/eal_md_method.c
    crypto/eal/src/eal_pkey_computesharekey.c
    crypto/eal/src/eal_pkey_crypt.c
    crypto/eal/src/eal_pkey_gen.c
    crypto/eal/src/eal_pkey_kem.c
    crypto/eal/src/eal_pkey_method.c
    crypto/eal/src/eal_pkey_sign.c
    crypto/eal/src/eal_rand.c
    crypto/eal/src/eal_rand_method.c
)

# Add module ealinit 
add_library(ealinit-objs OBJECT)
target_include_directories(ealinit-objs PRIVATE
    platform/Secure_C/include
    crypto/provider/include
    crypto/ealinit/include
)
target_sources(ealinit-objs PRIVATE
    crypto/ealinit/src/asmcap_alg_asm.c
    crypto/ealinit/src/cpucap.c
    crypto/ealinit/src/crypt_init.c
)

# Add module entropy 
add_library(entropy-objs OBJECT)
target_include_directories(entropy-objs PRIVATE
    platform/Secure_C/include
    bsl/list/include
    crypto/entropy/include
    bsl/sal/include
)
target_sources(entropy-objs PRIVATE
    crypto/entropy/src/entropy.c
    crypto/entropy/src/entropy_hardware.c
    crypto/entropy/src/entropy_seed_pool.c
    crypto/entropy/src/entropy_system.c
    crypto/entropy/src/es_cf.c
    crypto/entropy/src/es_cf_df.c
    crypto/entropy/src/es_entropy.c
    crypto/entropy/src/es_entropy_pool.c
    crypto/entropy/src/es_health_test.c
    crypto/entropy/src/es_noise_source.c
    crypto/entropy/src/es_ns_jitter.c
    crypto/entropy/src/es_ns_timestamp.c
)

# Add module hpke 
add_library(hpke-objs OBJECT)
target_include_directories(hpke-objs PRIVATE
    platform/Secure_C/include
    crypto/bn/include
    bsl/sal/include
)
target_sources(hpke-objs PRIVATE crypto/hpke/src/hpke.c)

# Add module scrypt 
add_library(scrypt-objs OBJECT)
target_include_directories(scrypt-objs PRIVATE
    crypto/eal/src
    platform/Secure_C/include
    crypto/scrypt/include
    crypto/pbkdf2/src
)
target_sources(scrypt-objs PRIVATE crypto/scrypt/src/scrypt.c)

# Add module hkdf 
add_library(hkdf-objs OBJECT)
target_include_directories(hkdf-objs PRIVATE
    crypto/hkdf/include
    platform/Secure_C/include
    crypto/eal/src
)
target_sources(hkdf-objs PRIVATE crypto/hkdf/src/hkdf.c)

# Add module pbkdf2 
add_library(pbkdf2-objs OBJECT)
target_include_directories(pbkdf2-objs PRIVATE
    crypto/eal/src
    platform/Secure_C/include
    crypto/pbkdf2/include
    crypto/ealinit/include
)
target_sources(pbkdf2-objs PRIVATE crypto/pbkdf2/src/pbkdf2.c)

# Add module kdf 
add_library(kdf-objs OBJECT)
target_include_directories(kdf-objs PRIVATE
    crypto/kdf/include
    platform/Secure_C/include
    crypto/eal/src
)
target_sources(kdf-objs PRIVATE crypto/kdf/src/kdf_tls12.c)

# Add module hmac 
add_library(hmac-objs OBJECT)
target_include_directories(hmac-objs PRIVATE
    crypto/eal/src
    platform/Secure_C/include
    crypto/hmac/include
)
target_sources(hmac-objs PRIVATE crypto/hmac/src/hmac.c)

# Add module gmac 
add_library(gmac-objs OBJECT)
target_include_directories(gmac-objs PRIVATE
    crypto/modes/include
    crypto/eal/src
    crypto/modes/src
    crypto/gmac/include
    platform/Secure_C/include
)
target_sources(gmac-objs PRIVATE crypto/gmac/src/gmac.c)

# Add module cmac 
add_library(cmac-objs OBJECT)
target_include_directories(cmac-objs PRIVATE
    crypto/eal/src
    platform/Secure_C/include
    crypto/cmac/src
    crypto/cmac/include
)
target_sources(cmac-objs PRIVATE
    crypto/cmac/src/cbc_mac.c
    crypto/cmac/src/cipher_mac_common.c
    crypto/cmac/src/cmac.c
)

# Add module siphash 
add_library(siphash-objs OBJECT)
target_include_directories(siphash-objs PRIVATE
    crypto/eal/src
    platform/Secure_C/include
    crypto/siphash/include
)
target_sources(siphash-objs PRIVATE crypto/siphash/src/siphash.c)

# Add module md5 
add_library(md5-objs OBJECT)
target_include_directories(md5-objs PRIVATE
    platform/Secure_C/include
    crypto/md5/include
)
target_sources(md5-objs PRIVATE
    crypto/md5/src/md5.c
    crypto/md5/src/noasm_md5.c
)

# Add module sm3 
add_library(sm3-objs OBJECT)
target_include_directories(sm3-objs PRIVATE
    platform/Secure_C/include
    crypto/sm3/include
)
target_sources(sm3-objs PRIVATE
    crypto/sm3/src/noasm_sm3.c
    crypto/sm3/src/sm3_public.c
)

# Add module sha1 
add_library(sha1-objs OBJECT)
target_include_directories(sha1-objs PRIVATE
    platform/Secure_C/include
    crypto/sha1/include
)
target_sources(sha1-objs PRIVATE
    crypto/sha1/src/noasm_sha1.c
    crypto/sha1/src/noasm_sha1_small.c
    crypto/sha1/src/sha1.c
)

# Add module sha2 
add_library(sha2-objs OBJECT)
target_include_directories(sha2-objs PRIVATE
    platform/Secure_C/include
    crypto/sha2/include
    bsl/sal/include
)
target_sources(sha2-objs PRIVATE
    crypto/sha2/src/noasm_sha256.c
    crypto/sha2/src/noasm_sha256_small.c
    crypto/sha2/src/noasm_sha512.c
    crypto/sha2/src/noasm_sha512_small.c
    crypto/sha2/src/sha2_256.c
    crypto/sha2/src/sha2_512.c
)

# Add module sha3 
add_library(sha3-objs OBJECT)
target_include_directories(sha3-objs PRIVATE
    platform/Secure_C/include
    crypto/sha3/include
)
target_sources(sha3-objs PRIVATE
    crypto/sha3/src/noasm_sha3.c
    crypto/sha3/src/sha3.c
)

# Add module modes 
add_library(modes-objs OBJECT)
target_include_directories(modes-objs PRIVATE
    crypto/modes/include
    crypto/sm4/include
    crypto/chacha20/include
    crypto/eal/src
    platform/Secure_C/include
    crypto/aes/include
    bsl/sal/include
)
target_sources(modes-objs PRIVATE
    crypto/modes/src/modes.c
    crypto/modes/src/modes_cbc.c
    crypto/modes/src/modes_ccm.c
    crypto/modes/src/modes_cfb.c
    crypto/modes/src/modes_chacha20_poly1305.c
    crypto/modes/src/modes_ctr.c
    crypto/modes/src/modes_ecb.c
    crypto/modes/src/modes_gcm.c
    crypto/modes/src/modes_ofb.c
    crypto/modes/src/modes_xts.c
    crypto/modes/src/noasm_aes_cbc.c
    crypto/modes/src/noasm_aes_ccm.c
    crypto/modes/src/noasm_aes_cfb.c
    crypto/modes/src/noasm_aes_ctr.c
    crypto/modes/src/noasm_aes_ecb.c
    crypto/modes/src/noasm_aes_gcm.c
    crypto/modes/src/noasm_aes_xts.c
    crypto/modes/src/noasm_ghash.c
    crypto/modes/src/noasm_poly1305.c
    crypto/modes/src/noasm_sm4_cbc.c
    crypto/modes/src/noasm_sm4_cfb.c
    crypto/modes/src/noasm_sm4_ctr.c
    crypto/modes/src/noasm_sm4_ecb.c
    crypto/modes/src/noasm_sm4_gcm.c
    crypto/modes/src/noasm_sm4_ofb.c
    crypto/modes/src/noasm_sm4_setkey.c
    crypto/modes/src/noasm_sm4_xts.c
)

# Add module ecc 
add_library(ecc-objs OBJECT)
target_include_directories(ecc-objs PRIVATE
    crypto/bn/include
    crypto/eal/src
    crypto/ecc/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(ecc-objs PRIVATE
    crypto/ecc/src/ecc.c
    crypto/ecc/src/ecc_method.c
    crypto/ecc/src/ecc_para.c
    crypto/ecc/src/ecc_pkey.c
    crypto/ecc/src/ecp_mont.c
    crypto/ecc/src/ecp_nist.c
    crypto/ecc/src/ecp_nistp224.c
    crypto/ecc/src/ecp_nistp521.c
    crypto/ecc/src/ecp_simple.c
    crypto/ecc/src/noasm_ecp_nistp256.c
)

# Add module rsa 
add_library(rsa-objs OBJECT)
target_include_directories(rsa-objs PRIVATE
    crypto/bn/include
    crypto/rsa/include
    crypto/eal/src
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(rsa-objs PRIVATE
    crypto/rsa/src/rsa_blinding.c
    crypto/rsa/src/rsa_ctrl.c
    crypto/rsa/src/rsa_encdec.c
    crypto/rsa/src/rsa_keygen.c
    crypto/rsa/src/rsa_keyop.c
    crypto/rsa/src/rsa_padding.c
)

# Add module dsa 
add_library(dsa-objs OBJECT)
target_include_directories(dsa-objs PRIVATE
    crypto/encode/include
    crypto/bn/include
    crypto/eal/src
    bsl/asn1/include
    bsl/obj/include
    platform/Secure_C/include
    crypto/dsa/include
    bsl/sal/include
)
target_sources(dsa-objs PRIVATE crypto/dsa/src/dsa_core.c)

# Add module encode 
add_library(encode-objs OBJECT)
target_include_directories(encode-objs PRIVATE
    crypto/encode/include
    crypto/bn/include
    bsl/asn1/include
    bsl/obj/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(encode-objs PRIVATE crypto/encode/src/crypt_encode.c)

# Add module dh 
add_library(dh-objs OBJECT)
target_include_directories(dh-objs PRIVATE
    crypto/bn/include
    crypto/dh/include
    crypto/eal/src
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(dh-objs PRIVATE
    crypto/dh/src/dh_core.c
    crypto/dh/src/dh_para.c
)

# Add module ecdh 
add_library(ecdh-objs OBJECT)
target_include_directories(ecdh-objs PRIVATE
    crypto/bn/include
    crypto/eal/src
    crypto/ecdh/include
    crypto/ecc/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(ecdh-objs PRIVATE crypto/ecdh/src/ecdh.c)

# Add module ecdsa 
add_library(ecdsa-objs OBJECT)
target_include_directories(ecdsa-objs PRIVATE
    crypto/encode/include
    crypto/bn/include
    crypto/ecdsa/include
    crypto/eal/src
    bsl/asn1/include
    bsl/obj/include
    crypto/ecc/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(ecdsa-objs PRIVATE crypto/ecdsa/src/ecdsa.c)

# Add module curve25519 
add_library(curve25519-objs OBJECT)
target_include_directories(curve25519-objs PRIVATE
    crypto/eal/src
    platform/Secure_C/include
    crypto/curve25519/include
    bsl/sal/include
)
target_sources(curve25519-objs PRIVATE
    crypto/curve25519/src/curve25519.c
    crypto/curve25519/src/curve25519_op.c
    crypto/curve25519/src/curve25519_table.c
    crypto/curve25519/src/noasm_curve25519_fp51_ops.c
)

# Add module sm2 
add_library(sm2-objs OBJECT)
target_include_directories(sm2-objs PRIVATE
    crypto/encode/include
    crypto/bn/include
    crypto/eal/src
    bsl/sal/include
    bsl/asn1/include
    bsl/obj/include
    crypto/ecc/include
    platform/Secure_C/include
    crypto/sm2/include
)
target_sources(sm2-objs PRIVATE
    crypto/sm2/src/sm2_crypt.c
    crypto/sm2/src/sm2_exch.c
    crypto/sm2/src/sm2_sign.c
)

# Add module mlkem 
add_library(mlkem-objs OBJECT)
target_include_directories(mlkem-objs PRIVATE
    crypto/eal/src
    crypto/sha3/include
    crypto/mlkem/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(mlkem-objs PRIVATE
    crypto/mlkem/src/ml_kem.c
    crypto/mlkem/src/ml_kem_ntt.c
    crypto/mlkem/src/ml_kem_pke.c
    crypto/mlkem/src/ml_kem_poly.c
)

# Add module mldsa 
add_library(mldsa-objs OBJECT)
target_include_directories(mldsa-objs PRIVATE
    crypto/eal/src
    crypto/sha3/include
    crypto/mldsa/include
    bsl/obj/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(mldsa-objs PRIVATE
    crypto/mldsa/src/ml_dsa.c
    crypto/mldsa/src/ml_dsa_core.c
    crypto/mldsa/src/ml_dsa_ntt.c
)

# Add module hybridkem 
add_library(hybridkem-objs OBJECT)
target_include_directories(hybridkem-objs PRIVATE
    crypto/bn/include
    crypto/curve25519/include
    crypto/hybridkem/include
    crypto/eal/src
    crypto/sha3/include
    crypto/mlkem/include
    crypto/ecdh/include
    crypto/ecc/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(hybridkem-objs PRIVATE crypto/hybridkem/src/crypt_hybridkem.c)

# Add module paillier 
add_library(paillier-objs OBJECT)
target_include_directories(paillier-objs PRIVATE
    crypto/bn/include
    crypto/eal/src
    crypto/paillier/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(paillier-objs PRIVATE
    crypto/paillier/src/paillier_encdec.c
    crypto/paillier/src/paillier_keygen.c
    crypto/paillier/src/paillier_keyop.c
)

# Add module elgamal 
add_library(elgamal-objs OBJECT)
target_include_directories(elgamal-objs PRIVATE
    crypto/bn/include
    crypto/eal/src
    crypto/elgamal/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(elgamal-objs PRIVATE
    crypto/elgamal/src/elgamal_encdec.c
    crypto/elgamal/src/elgamal_keygen.c
    crypto/elgamal/src/elgamal_keyop.c
    crypto/elgamal/src/originalroot.c
)

# Add module slh_dsa 
add_library(slh_dsa-objs OBJECT)
target_include_directories(slh_dsa-objs PRIVATE
    crypto/eal/src
    crypto/sha3/include
    bsl/asn1/include
    bsl/obj/include
    crypto/sha2/include
    platform/Secure_C/include
    crypto/slh_dsa/include
    bsl/sal/include
)
target_sources(slh_dsa-objs PRIVATE
    crypto/slh_dsa/src/slh_dsa.c
    crypto/slh_dsa/src/slh_dsa_fors.c
    crypto/slh_dsa/src/slh_dsa_hash.c
    crypto/slh_dsa/src/slh_dsa_hypertree.c
    crypto/slh_dsa/src/slh_dsa_wots.c
    crypto/slh_dsa/src/slh_dsa_xmss.c
)

# Add module provider 
add_library(provider-objs OBJECT)
target_include_directories(provider-objs PRIVATE
    crypto/modes/include
    crypto/bn/include
    crypto/hybridkem/include
    crypto/drbg/include
    crypto/eal/src
    crypto/mlkem/include
    crypto/md5/include
    crypto/sha2/include
    crypto/hmac/include
    crypto/include
    crypto/dh/include
    crypto/hkdf/include
    crypto/sha3/include
    crypto/elgamal/include
    bsl/hash/include
    crypto/ecdh/include
    crypto/siphash/include
    crypto/gmac/include
    crypto/sm3/include
    crypto/pbkdf2/include
    crypto/slh_dsa/include
    crypto/provider/include
    crypto/codecskey/include
    crypto/ealinit/include
    crypto/kdf/include
    crypto/scrypt/include
    crypto/mldsa/include
    crypto/ecc/include
    crypto/dsa/include
    crypto/sm2/include
    crypto/rsa/include
    crypto/curve25519/include
    crypto/ecdsa/include
    bsl/err/include
    crypto/entropy/include
    crypto/sha1/include
    crypto/paillier/include
    crypto/cmac/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(provider-objs PRIVATE
    crypto/provider/src/default/crypt_default_cipher.c
    crypto/provider/src/default/crypt_default_decode.c
    crypto/provider/src/default/crypt_default_kdf.c
    crypto/provider/src/default/crypt_default_kem.c
    crypto/provider/src/default/crypt_default_keyexch.c
    crypto/provider/src/default/crypt_default_keymgmt.c
    crypto/provider/src/default/crypt_default_mac.c
    crypto/provider/src/default/crypt_default_md.c
    crypto/provider/src/default/crypt_default_pkeycipher.c
    crypto/provider/src/default/crypt_default_provider.c
    crypto/provider/src/default/crypt_default_rand.c
    crypto/provider/src/default/crypt_default_sign.c
    crypto/provider/src/mgr/crypt_provider.c
    crypto/provider/src/mgr/crypt_provider_common.c
    crypto/provider/src/mgr/crypt_provider_compare.c
)

add_library(hitls_crypto-shared SHARED
    $<TARGET_OBJECTS:bn-objs>
    $<TARGET_OBJECTS:util-objs>
    $<TARGET_OBJECTS:aes-objs>
    $<TARGET_OBJECTS:sm4-objs>
    $<TARGET_OBJECTS:chacha20-objs>
    $<TARGET_OBJECTS:codecs-objs>
    $<TARGET_OBJECTS:codecskey-objs>
    $<TARGET_OBJECTS:drbg-objs>
    $<TARGET_OBJECTS:eal-objs>
    $<TARGET_OBJECTS:ealinit-objs>
    $<TARGET_OBJECTS:entropy-objs>
    $<TARGET_OBJECTS:hpke-objs>
    $<TARGET_OBJECTS:scrypt-objs>
    $<TARGET_OBJECTS:hkdf-objs>
    $<TARGET_OBJECTS:pbkdf2-objs>
    $<TARGET_OBJECTS:kdf-objs>
    $<TARGET_OBJECTS:hmac-objs>
    $<TARGET_OBJECTS:gmac-objs>
    $<TARGET_OBJECTS:cmac-objs>
    $<TARGET_OBJECTS:siphash-objs>
    $<TARGET_OBJECTS:md5-objs>
    $<TARGET_OBJECTS:sm3-objs>
    $<TARGET_OBJECTS:sha1-objs>
    $<TARGET_OBJECTS:sha2-objs>
    $<TARGET_OBJECTS:sha3-objs>
    $<TARGET_OBJECTS:modes-objs>
    $<TARGET_OBJECTS:ecc-objs>
    $<TARGET_OBJECTS:rsa-objs>
    $<TARGET_OBJECTS:dsa-objs>
    $<TARGET_OBJECTS:encode-objs>
    $<TARGET_OBJECTS:dh-objs>
    $<TARGET_OBJECTS:ecdh-objs>
    $<TARGET_OBJECTS:ecdsa-objs>
    $<TARGET_OBJECTS:curve25519-objs>
    $<TARGET_OBJECTS:sm2-objs>
    $<TARGET_OBJECTS:mlkem-objs>
    $<TARGET_OBJECTS:mldsa-objs>
    $<TARGET_OBJECTS:hybridkem-objs>
    $<TARGET_OBJECTS:paillier-objs>
    $<TARGET_OBJECTS:elgamal-objs>
    $<TARGET_OBJECTS:slh_dsa-objs>
    $<TARGET_OBJECTS:provider-objs>
)
target_link_options(hitls_crypto-shared PRIVATE ${SHARED_LNK_FLAGS})
set_target_properties(hitls_crypto-shared PROPERTIES OUTPUT_NAME hitls_crypto)
install(TARGETS hitls_crypto-shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(hitls_crypto-shared hitls_bsl-shared boundscheck)

add_library(hitls_crypto-static STATIC
    $<TARGET_OBJECTS:bn-objs>
    $<TARGET_OBJECTS:util-objs>
    $<TARGET_OBJECTS:aes-objs>
    $<TARGET_OBJECTS:sm4-objs>
    $<TARGET_OBJECTS:chacha20-objs>
    $<TARGET_OBJECTS:codecs-objs>
    $<TARGET_OBJECTS:codecskey-objs>
    $<TARGET_OBJECTS:drbg-objs>
    $<TARGET_OBJECTS:eal-objs>
    $<TARGET_OBJECTS:ealinit-objs>
    $<TARGET_OBJECTS:entropy-objs>
    $<TARGET_OBJECTS:hpke-objs>
    $<TARGET_OBJECTS:scrypt-objs>
    $<TARGET_OBJECTS:hkdf-objs>
    $<TARGET_OBJECTS:pbkdf2-objs>
    $<TARGET_OBJECTS:kdf-objs>
    $<TARGET_OBJECTS:hmac-objs>
    $<TARGET_OBJECTS:gmac-objs>
    $<TARGET_OBJECTS:cmac-objs>
    $<TARGET_OBJECTS:siphash-objs>
    $<TARGET_OBJECTS:md5-objs>
    $<TARGET_OBJECTS:sm3-objs>
    $<TARGET_OBJECTS:sha1-objs>
    $<TARGET_OBJECTS:sha2-objs>
    $<TARGET_OBJECTS:sha3-objs>
    $<TARGET_OBJECTS:modes-objs>
    $<TARGET_OBJECTS:ecc-objs>
    $<TARGET_OBJECTS:rsa-objs>
    $<TARGET_OBJECTS:dsa-objs>
    $<TARGET_OBJECTS:encode-objs>
    $<TARGET_OBJECTS:dh-objs>
    $<TARGET_OBJECTS:ecdh-objs>
    $<TARGET_OBJECTS:ecdsa-objs>
    $<TARGET_OBJECTS:curve25519-objs>
    $<TARGET_OBJECTS:sm2-objs>
    $<TARGET_OBJECTS:mlkem-objs>
    $<TARGET_OBJECTS:mldsa-objs>
    $<TARGET_OBJECTS:hybridkem-objs>
    $<TARGET_OBJECTS:paillier-objs>
    $<TARGET_OBJECTS:elgamal-objs>
    $<TARGET_OBJECTS:slh_dsa-objs>
    $<TARGET_OBJECTS:provider-objs>
)
set_target_properties(hitls_crypto-static PROPERTIES OUTPUT_NAME hitls_crypto)
install(TARGETS hitls_crypto-static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_executable(hitls_crypto-object
    $<TARGET_OBJECTS:bn-objs>
    $<TARGET_OBJECTS:util-objs>
    $<TARGET_OBJECTS:aes-objs>
    $<TARGET_OBJECTS:sm4-objs>
    $<TARGET_OBJECTS:chacha20-objs>
    $<TARGET_OBJECTS:codecs-objs>
    $<TARGET_OBJECTS:codecskey-objs>
    $<TARGET_OBJECTS:drbg-objs>
    $<TARGET_OBJECTS:eal-objs>
    $<TARGET_OBJECTS:ealinit-objs>
    $<TARGET_OBJECTS:entropy-objs>
    $<TARGET_OBJECTS:hpke-objs>
    $<TARGET_OBJECTS:scrypt-objs>
    $<TARGET_OBJECTS:hkdf-objs>
    $<TARGET_OBJECTS:pbkdf2-objs>
    $<TARGET_OBJECTS:kdf-objs>
    $<TARGET_OBJECTS:hmac-objs>
    $<TARGET_OBJECTS:gmac-objs>
    $<TARGET_OBJECTS:cmac-objs>
    $<TARGET_OBJECTS:siphash-objs>
    $<TARGET_OBJECTS:md5-objs>
    $<TARGET_OBJECTS:sm3-objs>
    $<TARGET_OBJECTS:sha1-objs>
    $<TARGET_OBJECTS:sha2-objs>
    $<TARGET_OBJECTS:sha3-objs>
    $<TARGET_OBJECTS:modes-objs>
    $<TARGET_OBJECTS:ecc-objs>
    $<TARGET_OBJECTS:rsa-objs>
    $<TARGET_OBJECTS:dsa-objs>
    $<TARGET_OBJECTS:encode-objs>
    $<TARGET_OBJECTS:dh-objs>
    $<TARGET_OBJECTS:ecdh-objs>
    $<TARGET_OBJECTS:ecdsa-objs>
    $<TARGET_OBJECTS:curve25519-objs>
    $<TARGET_OBJECTS:sm2-objs>
    $<TARGET_OBJECTS:mlkem-objs>
    $<TARGET_OBJECTS:mldsa-objs>
    $<TARGET_OBJECTS:hybridkem-objs>
    $<TARGET_OBJECTS:paillier-objs>
    $<TARGET_OBJECTS:elgamal-objs>
    $<TARGET_OBJECTS:slh_dsa-objs>
    $<TARGET_OBJECTS:provider-objs>
)
target_link_options(hitls_crypto-object PRIVATE ${PIE_EXE_LNK_FLAGS})
set_target_properties(hitls_crypto-object PROPERTIES OUTPUT_NAME libhitls_crypto.o)
install(TARGETS hitls_crypto-object DESTINATION ${CMAKE_INSTALL_PREFIX}/obj)


project(hitls_tls C)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_C_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_ASM_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENHITLS_VERSION_S='\"openHiTLS 0.2.0 15 May 2025\"' -DOPENHITLS_VERSION_I=2097152 -D__FILENAME__='\"$(notdir $(subst .o,,$@))\"'")
include_directories(
    tls/include
    bsl/err/include
    config/macro_config
    bsl/log/include
    include/tls
)

# Add module cm 
add_library(cm-objs OBJECT)
target_include_directories(cm-objs PRIVATE
    tls/cert/include
    tls/config/include
    tls/handshake/common/include
    tls/record/src
    tls/ccs/include
    tls/handshake/recv/include
    tls/feature/custom_extensions/include
    tls/handshake/include
    tls/crypt/include
    bsl/hash/include
    bsl/tlv/include
    tls/app/include
    tls/record/include
    bsl/uio/include
    tls/alert/include
    tls/cm/include
    tls/handshake/send/include
    include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(cm-objs PRIVATE
    tls/cm/src/conn_cert.c
    tls/cm/src/conn_common.c
    tls/cm/src/conn_create.c
    tls/cm/src/conn_ctrl.c
    tls/cm/src/conn_debug.c
    tls/cm/src/conn_establish.c
    tls/cm/src/conn_init.c
    tls/cm/src/conn_read.c
    tls/cm/src/conn_write.c
)

# Add module crypt 
add_library(crypt-objs OBJECT)
target_include_directories(crypt-objs PRIVATE
    tls/crypt/include
    tls/config/include
    tls/crypt/crypt_self
    platform/Secure_C/include
    tls/feature/custom_extensions/include
    bsl/sal/include
)
target_sources(crypt-objs PRIVATE tls/crypt/crypt_adapt/crypt.c)

# Add module crypt_self 
add_library(crypt_self-objs OBJECT)
target_include_directories(crypt_self-objs PRIVATE
    tls/crypt/include
    tls/config/include
    crypto/eal/include
    include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(crypt_self-objs PRIVATE
    tls/crypt/crypt_self/crypt_default.c
    tls/crypt/crypt_self/crypt_init.c
    tls/crypt/crypt_self/hitls_crypt.c
)

# Add module cert 
add_library(cert-objs OBJECT)
target_include_directories(cert-objs PRIVATE
    pki/x509_common/include
    tls/cert/include
    pki/x509_cert/include
    tls/config/include
    include/bsl
    bsl/hash/include
    include/pki
    bsl/asn1/include
    include
    include/tls
    tls/cert/cert_adapt
    platform/Secure_C/include
    tls/feature/custom_extensions/include
    bsl/sal/include
)
target_sources(cert-objs PRIVATE
    tls/cert/cert_adapt/cert.c
    tls/cert/cert_adapt/cert_chain.c
    tls/cert/cert_adapt/cert_method.c
    tls/cert/cert_adapt/cert_mgr_create.c
    tls/cert/cert_adapt/cert_mgr_ctrl.c
    tls/cert/cert_adapt/cert_pair.c
    tls/cert/hitls_x509_adapt/hitls_x509_cert_chain.c
    tls/cert/hitls_x509_adapt/hitls_x509_cert_magr.c
    tls/cert/hitls_x509_adapt/hitls_x509_cert_store.c
    tls/cert/hitls_x509_adapt/hitls_x509_crypto.c
    tls/cert/hitls_x509_adapt/hitls_x509_init.c
    tls/cert/hitls_x509_adapt/hitls_x509_pkey_magr.c
)

# Add module config 
add_library(config-objs OBJECT)
target_include_directories(config-objs PRIVATE
    tls/cert/include
    tls/crypt/include
    tls/config/include
    bsl/hash/include
    tls/record/include
    platform/Secure_C/include
    tls/feature/custom_extensions/include
    bsl/sal/include
)
target_sources(config-objs PRIVATE
    tls/config/src/cipher_suite.c
    tls/config/src/config.c
    tls/config/src/config_cert.c
    tls/config/src/config_check.c
    tls/config/src/config_default.c
    tls/config/src/config_group.c
    tls/config/src/config_sign.c
    tls/config/src/config_tls13.c
)

# Add module record 
add_library(record-objs OBJECT)
target_include_directories(record-objs PRIVATE
    tls/handshake/include
    tls/cert/include
    tls/crypt/include
    tls/alert/include
    tls/config/include
    bsl/hash/include
    bsl/tlv/include
    tls/record/include
    tls/handshake/common/include
    tls/feature/custom_extensions/include
    platform/Secure_C/include
    bsl/uio/include
    bsl/sal/include
)
target_sources(record-objs PRIVATE
    tls/record/src/rec_alert.c
    tls/record/src/rec_anti_replay.c
    tls/record/src/rec_buf.c
    tls/record/src/rec_conn.c
    tls/record/src/rec_crypto.c
    tls/record/src/rec_crypto_aead.c
    tls/record/src/rec_crypto_cbc.c
    tls/record/src/rec_read.c
    tls/record/src/rec_retransmit.c
    tls/record/src/rec_unprocessed_msg.c
    tls/record/src/rec_write.c
    tls/record/src/record.c
)

# Add module ccs 
add_library(ccs-objs OBJECT)
target_include_directories(ccs-objs PRIVATE
    tls/handshake/include
    tls/cert/include
    tls/crypt/include
    tls/alert/include
    tls/config/include
    bsl/hash/include
    bsl/tlv/include
    tls/record/include
    tls/feature/custom_extensions/include
    tls/ccs/include
    platform/Secure_C/include
    bsl/uio/include
    bsl/sal/include
)
target_sources(ccs-objs PRIVATE tls/ccs/src/change_cipher_spec.c)

# Add module alert 
add_library(alert-objs OBJECT)
target_include_directories(alert-objs PRIVATE
    tls/cert/include
    tls/crypt/include
    tls/alert/include
    tls/config/include
    bsl/hash/include
    bsl/tlv/include
    tls/record/include
    tls/record/src
    tls/feature/custom_extensions/include
    platform/Secure_C/include
    bsl/uio/include
    bsl/sal/include
)
target_sources(alert-objs PRIVATE tls/alert/src/alert.c)

# Add module handshake 
add_library(handshake-objs OBJECT)
target_include_directories(handshake-objs PRIVATE
    tls/cert/include
    tls/config/include
    tls/handshake/common/include
    tls/record/src
    tls/handshake/recv/include
    tls/cert/cert_adapt
    tls/feature/custom_extensions/include
    tls/handshake/include
    tls/crypt/include
    bsl/hash/include
    bsl/tlv/include
    tls/handshake/pack/include
    tls/handshake/reass/include
    tls/record/include
    bsl/uio/include
    tls/include
    tls/alert/include
    tls/handshake/parse/include
    tls/handshake/send/include
    platform/Secure_C/include
    tls/handshake/cookie/include
    bsl/sal/include
)
target_sources(handshake-objs PRIVATE
    tls/handshake/common/src/hs_common.c
    tls/handshake/common/src/hs_dtls_timer.c
    tls/handshake/common/src/hs_kx.c
    tls/handshake/common/src/hs_verify.c
    tls/handshake/common/src/tls13key.c
    tls/handshake/common/src/transcript_hash.c
    tls/handshake/cookie/src/hs_cookie.c
    tls/handshake/pack/src/pack.c
    tls/handshake/pack/src/pack_certificate.c
    tls/handshake/pack/src/pack_certificate_request.c
    tls/handshake/pack/src/pack_certificate_verify.c
    tls/handshake/pack/src/pack_client_hello.c
    tls/handshake/pack/src/pack_client_key_exchange.c
    tls/handshake/pack/src/pack_common.c
    tls/handshake/pack/src/pack_encrypted_extensions.c
    tls/handshake/pack/src/pack_extensions.c
    tls/handshake/pack/src/pack_finished.c
    tls/handshake/pack/src/pack_hello_verify_request.c
    tls/handshake/pack/src/pack_key_update.c
    tls/handshake/pack/src/pack_new_session_ticket.c
    tls/handshake/pack/src/pack_server_hello.c
    tls/handshake/pack/src/pack_server_key_exchange.c
    tls/handshake/parse/src/parse.c
    tls/handshake/parse/src/parse_certificate.c
    tls/handshake/parse/src/parse_certificate_request.c
    tls/handshake/parse/src/parse_certificate_verify.c
    tls/handshake/parse/src/parse_client_hello.c
    tls/handshake/parse/src/parse_client_key_exchange.c
    tls/handshake/parse/src/parse_common.c
    tls/handshake/parse/src/parse_encrypted_extensions.c
    tls/handshake/parse/src/parse_extensions.c
    tls/handshake/parse/src/parse_extensions_client.c
    tls/handshake/parse/src/parse_extensions_server.c
    tls/handshake/parse/src/parse_finished.c
    tls/handshake/parse/src/parse_hello_verify_request.c
    tls/handshake/parse/src/parse_key_update.c
    tls/handshake/parse/src/parse_new_sesion_ticket.c
    tls/handshake/parse/src/parse_server_hello.c
    tls/handshake/parse/src/parse_server_key_exchange.c
    tls/handshake/reass/src/hs_reass.c
    tls/handshake/recv/src/hs_state_recv.c
    tls/handshake/recv/src/recv_cert_request.c
    tls/handshake/recv/src/recv_cert_verify.c
    tls/handshake/recv/src/recv_certificate.c
    tls/handshake/recv/src/recv_client_hello.c
    tls/handshake/recv/src/recv_client_key_exchange.c
    tls/handshake/recv/src/recv_encrypted_extensions.c
    tls/handshake/recv/src/recv_finished.c
    tls/handshake/recv/src/recv_hello_verify_request.c
    tls/handshake/recv/src/recv_new_session_ticket.c
    tls/handshake/recv/src/recv_server_hello.c
    tls/handshake/recv/src/recv_server_hello_done.c
    tls/handshake/recv/src/recv_server_key_exchange.c
    tls/handshake/send/src/hs_state_send.c
    tls/handshake/send/src/send_cert_request.c
    tls/handshake/send/src/send_cert_verify.c
    tls/handshake/send/src/send_certificate.c
    tls/handshake/send/src/send_change_cipher_spec.c
    tls/handshake/send/src/send_client_hello.c
    tls/handshake/send/src/send_client_key_exchange.c
    tls/handshake/send/src/send_common.c
    tls/handshake/send/src/send_encrypted_extensions.c
    tls/handshake/send/src/send_finished.c
    tls/handshake/send/src/send_hello_request.c
    tls/handshake/send/src/send_hello_verify_request.c
    tls/handshake/send/src/send_new_session_ticket.c
    tls/handshake/send/src/send_server_hello.c
    tls/handshake/send/src/send_server_hello_done.c
    tls/handshake/send/src/send_server_key_exchange.c
    tls/handshake/sm/src/hs_init.c
    tls/handshake/sm/src/hs_sm.c
)

# Add module app 
add_library(app-objs OBJECT)
target_include_directories(app-objs PRIVATE
    tls/app/include
    tls/cert/include
    tls/crypt/include
    tls/config/include
    bsl/hash/include
    bsl/tlv/include
    tls/record/include
    tls/record/src
    tls/feature/custom_extensions/include
    platform/Secure_C/include
    bsl/uio/include
    bsl/sal/include
)
target_sources(app-objs PRIVATE tls/app/src/app.c)

# Add module feature 
add_library(feature-objs OBJECT)
target_include_directories(feature-objs PRIVATE
    tls/cert/include
    tls/config/include
    tls/handshake/common/include
    tls/ccs/include
    tls/cert/cert_adapt
    tls/feature/custom_extensions/include
    tls/handshake/include
    tls/crypt/include
    bsl/hash/include
    bsl/tlv/include
    bsl/hash/include/
    tls/app/include
    tls/handshake/parse/src
    tls/record/include
    bsl/uio/include
    tls/alert/include
    tls/cm/include
    bsl/uio/src
    include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(feature-objs PRIVATE
    tls/feature/alpn/src/alpn.c
    tls/feature/custom_extensions/src/custom_extensions.c
    tls/feature/indicator/src/indicator.c
    tls/feature/security/src/security.c
    tls/feature/security/src/security_default.c
    tls/feature/session/src/session.c
    tls/feature/session/src/session_dec.c
    tls/feature/session/src/session_enc.c
    tls/feature/session/src/session_mgr.c
    tls/feature/session/src/session_ticket.c
    tls/feature/sni/src/sni.c
)

add_library(hitls_tls-shared SHARED
    $<TARGET_OBJECTS:cm-objs>
    $<TARGET_OBJECTS:crypt-objs>
    $<TARGET_OBJECTS:crypt_self-objs>
    $<TARGET_OBJECTS:cert-objs>
    $<TARGET_OBJECTS:config-objs>
    $<TARGET_OBJECTS:record-objs>
    $<TARGET_OBJECTS:ccs-objs>
    $<TARGET_OBJECTS:alert-objs>
    $<TARGET_OBJECTS:handshake-objs>
    $<TARGET_OBJECTS:app-objs>
    $<TARGET_OBJECTS:feature-objs>
)
target_link_options(hitls_tls-shared PRIVATE ${SHARED_LNK_FLAGS})
set_target_properties(hitls_tls-shared PROPERTIES OUTPUT_NAME hitls_tls)
install(TARGETS hitls_tls-shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(hitls_tls-shared hitls_bsl-shared boundscheck)

add_library(hitls_tls-static STATIC
    $<TARGET_OBJECTS:cm-objs>
    $<TARGET_OBJECTS:crypt-objs>
    $<TARGET_OBJECTS:crypt_self-objs>
    $<TARGET_OBJECTS:cert-objs>
    $<TARGET_OBJECTS:config-objs>
    $<TARGET_OBJECTS:record-objs>
    $<TARGET_OBJECTS:ccs-objs>
    $<TARGET_OBJECTS:alert-objs>
    $<TARGET_OBJECTS:handshake-objs>
    $<TARGET_OBJECTS:app-objs>
    $<TARGET_OBJECTS:feature-objs>
)
set_target_properties(hitls_tls-static PROPERTIES OUTPUT_NAME hitls_tls)
install(TARGETS hitls_tls-static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_executable(hitls_tls-object
    $<TARGET_OBJECTS:cm-objs>
    $<TARGET_OBJECTS:crypt-objs>
    $<TARGET_OBJECTS:crypt_self-objs>
    $<TARGET_OBJECTS:cert-objs>
    $<TARGET_OBJECTS:config-objs>
    $<TARGET_OBJECTS:record-objs>
    $<TARGET_OBJECTS:ccs-objs>
    $<TARGET_OBJECTS:alert-objs>
    $<TARGET_OBJECTS:handshake-objs>
    $<TARGET_OBJECTS:app-objs>
    $<TARGET_OBJECTS:feature-objs>
)
target_link_options(hitls_tls-object PRIVATE ${PIE_EXE_LNK_FLAGS})
set_target_properties(hitls_tls-object PROPERTIES OUTPUT_NAME libhitls_tls.o)
install(TARGETS hitls_tls-object DESTINATION ${CMAKE_INSTALL_PREFIX}/obj)


project(hitls_pki C)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_C_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_ASM_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENHITLS_VERSION_S='\"openHiTLS 0.2.0 15 May 2025\"' -DOPENHITLS_VERSION_I=2097152 -D__FILENAME__='\"$(notdir $(subst .o,,$@))\"'")
include_directories(
    include/pki
    bsl/err/include
    config/macro_config
    bsl/log/include
)

# Add module print 
add_library(print-objs OBJECT)
target_include_directories(print-objs PRIVATE
    pki/x509_common/include
    pki/print/include
    bsl/list/include
    bsl/asn1/include
    bsl/obj/include
    platform/Secure_C/include
    bsl/uio/include
    bsl/sal/include
)
target_sources(print-objs PRIVATE pki/print/src/hitls_pki_print.c)

# Add module x509_common 
add_library(x509_common-objs OBJECT)
target_include_directories(x509_common-objs PRIVATE
    pki/x509_common/include
    bsl/list/include
    crypto/codecskey/include
    bsl/asn1/include
    bsl/obj/include
    bsl/pem/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(x509_common-objs PRIVATE
    pki/x509_common/src/hitls_x509_attrs.c
    pki/x509_common/src/hitls_x509_common.c
    pki/x509_common/src/hitls_x509_ctrl.c
    pki/x509_common/src/hitls_x509_ext.c
)

# Add module pkcs12 
add_library(pkcs12-objs OBJECT)
target_include_directories(pkcs12-objs PRIVATE
    pki/x509_common/include
    pki/x509_cert/include
    pki/pkcs12/include
    bsl/list/include
    crypto/codecskey/include
    bsl/asn1/include
    bsl/obj/include
    pki/cms/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(pkcs12-objs PRIVATE
    pki/pkcs12/src/hitls_pkcs12_common.c
    pki/pkcs12/src/hitls_pkcs12_util.c
)

# Add module cms 
add_library(cms-objs OBJECT)
target_include_directories(cms-objs PRIVATE
    crypto/codecskey/include
    bsl/asn1/include
    bsl/obj/include
    pki/cms/include
    platform/Secure_C/include
    bsl/sal/include
)
target_sources(cms-objs PRIVATE pki/cms/src/hitls_cms_common.c)

# Add module x509_cert 
add_library(x509_cert-objs OBJECT)
target_include_directories(x509_cert-objs PRIVATE
    pki/x509_common/include
    pki/x509_cert/include
    pki/print/include
    bsl/list/include
    crypto/codecskey/include
    bsl/asn1/include
    bsl/obj/include
    bsl/pem/include
    platform/Secure_C/include
    pki/x509_csr/include
    bsl/sal/include
)
target_sources(x509_cert-objs PRIVATE pki/x509_cert/src/hitls_x509_cert.c)

# Add module x509_crl 
add_library(x509_crl-objs OBJECT)
target_include_directories(x509_crl-objs PRIVATE
    pki/x509_common/include
    bsl/list/include
    crypto/codecskey/include
    bsl/asn1/include
    bsl/obj/include
    bsl/pem/include
    platform/Secure_C/include
    pki/x509_crl/include
    bsl/sal/include
)
target_sources(x509_crl-objs PRIVATE pki/x509_crl/src/hitls_x509_crl.c)

# Add module x509_csr 
add_library(x509_csr-objs OBJECT)
target_include_directories(x509_csr-objs PRIVATE
    pki/x509_common/include
    bsl/list/include
    crypto/codecskey/include
    bsl/asn1/include
    bsl/obj/include
    bsl/pem/include
    platform/Secure_C/include
    pki/x509_csr/include
    bsl/sal/include
)
target_sources(x509_csr-objs PRIVATE pki/x509_csr/src/hitls_x509_csr.c)

# Add module x509_verify 
add_library(x509_verify-objs OBJECT)
target_include_directories(x509_verify-objs PRIVATE
    pki/x509_common/include
    pki/x509_cert/include
    bsl/list/include
    crypto/codecskey/include
    bsl/asn1/include
    bsl/obj/include
    platform/Secure_C/include
    pki/x509_verify/include
    pki/x509_crl/include
    bsl/sal/include
)
target_sources(x509_verify-objs PRIVATE pki/x509_verify/src/hitls_x509_verify.c)

add_library(hitls_pki-shared SHARED
    $<TARGET_OBJECTS:print-objs>
    $<TARGET_OBJECTS:x509_common-objs>
    $<TARGET_OBJECTS:pkcs12-objs>
    $<TARGET_OBJECTS:cms-objs>
    $<TARGET_OBJECTS:x509_cert-objs>
    $<TARGET_OBJECTS:x509_crl-objs>
    $<TARGET_OBJECTS:x509_csr-objs>
    $<TARGET_OBJECTS:x509_verify-objs>
)
target_link_options(hitls_pki-shared PRIVATE ${SHARED_LNK_FLAGS})
set_target_properties(hitls_pki-shared PROPERTIES OUTPUT_NAME hitls_pki)
install(TARGETS hitls_pki-shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(hitls_pki-shared hitls_crypto-shared hitls_bsl-shared boundscheck)

add_library(hitls_pki-static STATIC
    $<TARGET_OBJECTS:print-objs>
    $<TARGET_OBJECTS:x509_common-objs>
    $<TARGET_OBJECTS:pkcs12-objs>
    $<TARGET_OBJECTS:cms-objs>
    $<TARGET_OBJECTS:x509_cert-objs>
    $<TARGET_OBJECTS:x509_crl-objs>
    $<TARGET_OBJECTS:x509_csr-objs>
    $<TARGET_OBJECTS:x509_verify-objs>
)
set_target_properties(hitls_pki-static PROPERTIES OUTPUT_NAME hitls_pki)
install(TARGETS hitls_pki-static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_executable(hitls_pki-object
    $<TARGET_OBJECTS:print-objs>
    $<TARGET_OBJECTS:x509_common-objs>
    $<TARGET_OBJECTS:pkcs12-objs>
    $<TARGET_OBJECTS:cms-objs>
    $<TARGET_OBJECTS:x509_cert-objs>
    $<TARGET_OBJECTS:x509_crl-objs>
    $<TARGET_OBJECTS:x509_csr-objs>
    $<TARGET_OBJECTS:x509_verify-objs>
)
target_link_options(hitls_pki-object PRIVATE ${PIE_EXE_LNK_FLAGS})
set_target_properties(hitls_pki-object PROPERTIES OUTPUT_NAME libhitls_pki.o)
install(TARGETS hitls_pki-object DESTINATION ${CMAKE_INSTALL_PREFIX}/obj)


project(hitls_auth C)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_C_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_ASM_FLAGS ${CC_ALL_OPTIONS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENHITLS_VERSION_S='\"openHiTLS 0.2.0 15 May 2025\"' -DOPENHITLS_VERSION_I=2097152 -D__FILENAME__='\"$(notdir $(subst .o,,$@))\"'")
include_directories(
    include/auth
    bsl/err/include
    config/macro_config
    bsl/log/include
)

# Add module privpass_token 
add_library(privpass_token-objs OBJECT)
target_include_directories(privpass_token-objs PRIVATE
    platform/Secure_C/include
    auth/privpass_token/include
    bsl/sal/include
)
target_sources(privpass_token-objs PRIVATE
    auth/privpass_token/src/privpass_token.c
    auth/privpass_token/src/privpass_token_util.c
    auth/privpass_token/src/privpass_token_wrapper.c
)

add_library(hitls_auth-shared SHARED $<TARGET_OBJECTS:privpass_token-objs>)
target_link_options(hitls_auth-shared PRIVATE ${SHARED_LNK_FLAGS})
set_target_properties(hitls_auth-shared PROPERTIES OUTPUT_NAME hitls_auth)
install(TARGETS hitls_auth-shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
target_link_libraries(hitls_auth-shared hitls_crypto-shared hitls_bsl-shared boundscheck)

add_library(hitls_auth-static STATIC $<TARGET_OBJECTS:privpass_token-objs>)
set_target_properties(hitls_auth-static PROPERTIES OUTPUT_NAME hitls_auth)
install(TARGETS hitls_auth-static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_executable(hitls_auth-object $<TARGET_OBJECTS:privpass_token-objs>)
target_link_options(hitls_auth-object PRIVATE ${PIE_EXE_LNK_FLAGS})
set_target_properties(hitls_auth-object PROPERTIES OUTPUT_NAME libhitls_auth.o)
install(TARGETS hitls_auth-object DESTINATION ${CMAKE_INSTALL_PREFIX}/obj)


add_custom_target(openHiTLS)
add_dependencies(openHiTLS
    hitls_bsl-shared
    hitls_bsl-static
    hitls_bsl-object
    hitls_crypto-shared
    hitls_crypto-static
    hitls_crypto-object
    hitls_tls-shared
    hitls_tls-static
    hitls_tls-object
    hitls_pki-shared
    hitls_pki-static
    hitls_pki-object
    hitls_auth-shared
    hitls_auth-static
    hitls_auth-object
)
