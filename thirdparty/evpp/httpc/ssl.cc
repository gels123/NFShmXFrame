#include "evpp/httpc/ssl.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"

#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
#include "evpp/logging.h"

#include <openssl/rand.h>
#include <openssl/err.h>

namespace evpp {
namespace httpc {
SSL_CTX* g_ssl_ctx = nullptr;

bool InitSSL() {
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    int r = RAND_poll();
    if (r == 0) {
        EVPP_LOG_ERROR << "RAND_poll failed";
        NFLogError(NF_LOG_DEFAULT, 0, "RAND_poll failed");
        return false;
    }
    g_ssl_ctx = SSL_CTX_new(SSLv23_method());
    if (!g_ssl_ctx) {
        EVPP_LOG_ERROR << "SSL_CTX_new failed";
        NFLogError(NF_LOG_DEFAULT, 0, "SSL_CTX_new failed");
        return false;
    }
    X509_STORE* store = SSL_CTX_get_cert_store(g_ssl_ctx);
    if (X509_STORE_load_locations(store, "./cacert.pem", NULL) != 1) {
        EVPP_LOG_ERROR << "X509_STORE_set_default_paths failed";
        NFLogError(NF_LOG_DEFAULT, 0, "X509_STORE_set_default_paths failed");
        return false;
    }
    return true;
}

void CleanSSL() {
    if (g_ssl_ctx != nullptr) {
        SSL_CTX_free(g_ssl_ctx);
    }
    ERR_free_strings();
    EVP_cleanup();
    ERR_remove_thread_state(nullptr);
    CRYPTO_cleanup_all_ex_data();
}

SSL_CTX* GetSSLCtx() {
    return g_ssl_ctx;
}
} // httpc
} // evpp

#endif
