#include "webserver.hpp"

/* System. */
#include <stdio.h>

/* LWIP. */
#include "lwipopts.h"
#include "lwip/tcp.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "lwip/apps/mdns.h"

/* Pico. */
#include "pico/cyw43_arch.h"

/* OpenACE. */
#include "ace/coreutils.hpp"

Webserver *webserver;

/** Needed for captive portal */
extern struct tcp_pcb *tcp_input_pcb;
inline etl::map<uint32_t, uint32_t, 4> captiveCheck;

/* Other consts */
constexpr char X_OPENACE_METHOD[] = "X-Method: ";    // A method to tel our custom POST received what we actually wanted to do
constexpr char CONFIGPATH[] = "/api/_Configuration"; // Should be the same as api/<Configuration::NAME>

static struct RequestContext_t
{
    void *current_connection;
    OpenAce::ConfigPathString uri;
    enum
    {
        POST,
        DELETE
    } method;
    bool response;
} RequestContext;

// ******************************************
//  Post handling
// ******************************************

#define ARR(...) {__VA_ARGS__}

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                       u16_t http_request_len, int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd)
{
    LWIP_UNUSED_ARG(connection);
    LWIP_UNUSED_ARG(http_request);
    LWIP_UNUSED_ARG(http_request_len);
    LWIP_UNUSED_ARG(content_len);
    LWIP_UNUSED_ARG(post_auto_wnd);
    if (strncmp(uri, CONFIGPATH, sizeof(CONFIGPATH) - 1) == 0)
    {
        if (RequestContext.current_connection != connection)
        {
            *post_auto_wnd = 1; // Must be set to 1

            RequestContext =
            {
                .current_connection = connection,
                .uri = uri,
                .method = RequestContext_t::POST,
                .response = false
            };

            //            etl::string_ext text(const_cast<char*>(http_request), http_request, http_request_len);
            auto method = strstr(http_request, X_OPENACE_METHOD);
            if (method)
            {
                method += sizeof(X_OPENACE_METHOD) - 1;
                if (strncmp(method, "DELETE", sizeof("DELETE") - 1) == 0)
                {
                    RequestContext.method = RequestContext_t::DELETE;
                }
            }
            return ERR_OK;
        }
    }
    return ERR_OK;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p)
{
    err_t retval = ERR_VAL;
    if (RequestContext.current_connection == connection)
    {
        char buffer[256];
        char *buf = (char *)pbuf_get_contiguous(p, buffer, sizeof(buffer), p->tot_len, 0);
        Configuration *configModule = static_cast<Configuration *>(BaseModule::moduleByName(*webserver, Configuration::NAME, false));
        if (configModule != nullptr)
        {
            if (RequestContext.method == RequestContext_t::POST && buf)
            {
                // Handle update configuration requests
                buf[p->tot_len] = '\0';
                etl::string_ext requestData(buf, buf, p->tot_len + 1);
                RequestContext.response = configModule->setData(requestData, RequestContext.uri);
                retval = ERR_OK;
            }
            else if (RequestContext.method == RequestContext_t::DELETE)
                // Handle delete configuraiton requests
            {
                RequestContext.response = configModule->deleteData(RequestContext.uri);
                retval = ERR_OK;
            }

            // Inform the module that the configuration has been updated
            // TODO: Is RequestContext.response sometimes false??
            if (RequestContext.response)
            {
                auto pathParsed = CoreUtils::parsePath(RequestContext.uri);
                configModule->getBus().receive(
                    OpenAce::ConfigUpdatedMsg
                {
                    *configModule,
                    pathParsed[2],
                });
            }
        }
    }
    pbuf_free(p);
    return retval;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{
    if (RequestContext.current_connection == connection)
    {
        //        if (RequestContext.response)
        //      {
        snprintf(response_uri, response_uri_len, "/ok.json");
        //      }
        //      else
        //    {
        //    snprintf(response_uri, response_uri_len, "/error.json");
        //  }
        RequestContext.current_connection = NULL;
    }
}

// ******************************************
// /API Get handling
// ******************************************
extern struct fsdata_file file_captive_html[];
extern struct fsdata_file file_ios_html[];
const uint8_t IS_ACCEPTED = 1 << 1;

int handle_captive(fs_file *file, const etl::ivector<OpenAce::Modulename> &path)
{
    // How captive works. All devices test a URL and see what content is returned. If the unexpected content is returned, it's assumed it need to be captive:
    // For iOS: iOS loads hotspot-detect.html -> OpenAce shows something different -> iOS Opens loading the page OpenAce returns a landing page with redirect -> iOS still showing captive, follows redirect and shows OpenACE
    // for Android: Android checks generate_204 -> OpenAce returns something different -> Keep portal open, but service pages
    // For Android it would be nice to start using DHCP_OPT_CAPTIVE_PORTAL see dhcpserver.c
    uint32_t remoteAddress = tcp_input_pcb->remote_ip.addr;
    if (captiveCheck[remoteAddress] & IS_ACCEPTED)
    {
        return 0;
    }

    // if (!(captiveCheck[remoteAddress] & IS_CAPTIVE))
    // {
    //     const etl::vector captivePaths{ "generate_204", "gen_204", "hotspot-detect", "ios", "captive" };
    //     // Search for the pathFront in the captivePaths vector
    //     if (etl::find(captivePaths.cbegin(), captivePaths.cend(), path.front()) != captivePaths.cend())
    //     {
    //         captiveCheck[remoteAddress] |= IS_CAPTIVE;
    //     }
    // }

    const struct fsdata_file *captiveDocument;
    if (path.front() == "hotspot-detect") // IOS, set to accepted so the connection is accepted
    {
        captiveDocument = &file_ios_html[0];
        captiveCheck[remoteAddress] |= IS_ACCEPTED;
    }
    else if (path.front() == "generate_204" || path.front() == "gen_204") // Android, accept the captive.
    {
        captiveDocument = &file_captive_html[0];
        captiveCheck[remoteAddress] |= IS_ACCEPTED;
    }
    else
    {
        return 0;
    }

    // Setup file for sending
    file->data = (const char *)captiveDocument[0].data;
    file->len = captiveDocument[0].len;
    file->index = captiveDocument[0].len;
    file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
    return 1;
}

int fs_open_custom(fs_file *file, const char *name)
{
    constexpr auto MAX_CONTENT_SIZE = 1025;

    auto pathString = OpenAce::ConfigPathString(name);
    auto path = CoreUtils::parsePath(pathString);

    // Test if this is an request for api data requests
    if (path.size() == 0)
    {
        return 0;
    }

    // Test and handle captive portal
    // if (handle_captive(file, path))
    // {
    //     return 1;
    // }

    // OpenAce API calls
    if (path.back() != "json" || path.front() != "api")
    {
        return 0;
    }

    BaseModule *module = BaseModule::moduleByName(*webserver, path[1].c_str(), false);
    if (module == nullptr)
    {
        return 0;
    }

    memset(file, 0, sizeof(fs_file));
    file->pextension = mem_malloc(MAX_CONTENT_SIZE + 1);
    if (file->pextension)
    {
        // Get the response and get it's size in the large buffer first
        etl::string_ext response((char *)file->pextension, MAX_CONTENT_SIZE);
        etl::string_stream stream(response);
        module->getData(stream, pathString);
        int length = response.size();

        file->data = (const char *)file->pextension;
        file->len = length;
        file->index = file->len; // We set index to len to indicate that the complete file has been read and the server can send and close the response
        // file->flags = FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_HTTPVER_1_1 | FS_FILE_FLAGS_HEADER_PERSISTENT;

        return 1;
    }
    return 0;
}

void fs_close_custom(fs_file *file)
{
    if (file && file->pextension)
    {
        mem_free(file->pextension);
    }
}

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
    puts("not implemented in this example configuration");
    LWIP_ASSERT("not implemented in this example configuration", 0);
    LWIP_UNUSED_ARG(file);
    LWIP_UNUSED_ARG(buffer);
    LWIP_UNUSED_ARG(count);
    /* Return
       - FS_READ_EOF if all bytes have been read
       - FS_READ_DELAYED if reading is delayed (call 'tcpip_callback(callback_fn, callback_arg)' when done) */
    /* all bytes read already */
    return FS_READ_EOF;
}

void Webserver::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    const auto registeredModules = BaseModule::registeredModules();
    stream << "{";
    stream << "\"modules\":{";

    for (auto it = registeredModules.cbegin(); it != registeredModules.cend(); it++)
    {
        stream << "\"" << it->first << "\": {\"poststatus\": " << (uint8_t)(it->second.result) << "}";
        if (etl::next(it) != registeredModules.cend())
        {
            stream << ",";
        }
    }

    stream << "}";
    stream << "}\n";
}

OpenAce::PostConstruct Webserver::postConstruct()
{
    return OpenAce::PostConstruct::OK;
}

void Webserver::start()
{
    webserver = this;
    httpd_init();
    getBus().subscribe(*this);
};

void Webserver::stop()
{
    getBus().unsubscribe(*this);
};

void Webserver::on_receive_unknown(const etl::imessage &msg)
{
}
