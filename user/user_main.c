#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"
#include "bme280.h"
#include "debug.h"
#include "util.h"

struct espconn server_conn; // server connection
ip_addr_t server_ip; // server ip address
esp_tcp server_tcp; // server tcp connection

char json_data[ 256 ];
char dt[ 32 ];
char dp[ 32 ];
char dh[ 32 ];
char buffer[ 2048 ];

void user_rf_pre_init( void )
{
}

/* http://www.esp8266.com/wiki/doku.php?id=esp8266_power_usage */
void goto_sleep() {
        os_printf( "Going to sleep for %d seconds..\n", SLEEP_SEC);
        deep_sleep_set_option( 0 );
        system_deep_sleep( SLEEP_SEC * 1000 * 1000 );
}



void data_received( void *arg, char *pdata, unsigned short len )
{
        struct espconn *conn = arg;
        os_printf( "%s: %s\n", __FUNCTION__, pdata );
        espconn_disconnect( conn );
}


void tcp_connected( void *arg )
{

        os_printf( "%s\n", __FUNCTION__ );

        struct sensor_result sr;
        read_all(&sr);
        os_sprintf( dt, "Temperature: %s\n", f2s(sr.temperature,2));
        os_sprintf( dh, "Humidity: %s\n", f2s(sr.humidity,2));
        os_sprintf( dp, "Pressure: %s\n", f2s(sr.pressure,2));

        struct espconn *conn = arg;

        espconn_regist_recvcb( conn, data_received );

        char *remote_hostname;
        #if USE_DNS
          remote_hostname = REMOTE_HOST;
        #else
          remote_hostname = REMOTE_IP;
        #endif
        os_sprintf( json_data, "{\"temperature\": \"%s\", \"humidity\": \"%s\", \"pressure\": \"%s\"}", dt, dh, dp );
        os_sprintf( buffer, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
                    DATA_URL, remote_hostname, os_strlen( json_data ), json_data );

        os_printf( "Sending: %s\n", buffer );
        espconn_sent( conn, buffer, os_strlen( buffer ) );
        espconn_disconnect( conn );

}


void tcp_disconnected( void *arg )
{
        struct espconn *conn = arg;
        os_printf( "%s\n", __FUNCTION__ );
        wifi_station_disconnect();
}

void tcp_error(void *arg, sint8 err) {
        os_printf("Error connecting: %d\n", err);
        wifi_station_disconnect();
}

void dns_done( const char *name, ip_addr_t *ipaddr, void *arg )
{
        struct espconn *conn = arg;

        os_printf( "%s\n", __FUNCTION__ );

        if ( ipaddr == NULL)
        {
                os_printf("DNS lookup failed\n");
                wifi_station_disconnect();
        }
        else
        {
                os_printf("Connecting...\n" );

                conn->type = ESPCONN_TCP;
                conn->state = ESPCONN_NONE;
                conn->proto.tcp=&server_tcp;
                conn->proto.tcp->local_port = espconn_port();
                conn->proto.tcp->remote_port = 80;
                os_memcpy( conn->proto.tcp->remote_ip, &ipaddr->addr, 4 );

                espconn_regist_connectcb( conn, tcp_connected );
                espconn_regist_disconcb( conn, tcp_disconnected );

                espconn_connect( conn );
        }
}

/* send data to specified ip address */
void to_ip_address( )
{
        struct espconn *conn = &server_conn;
        ipaddr_aton(REMOTE_IP, &server_ip);
        ip_addr_t *target = &server_ip;

        os_printf( "%s\n", __FUNCTION__ );

        os_printf("Connecting to IP address %s...\n", REMOTE_IP );

        conn->type = ESPCONN_TCP;
        conn->state = ESPCONN_NONE;
        conn->proto.tcp=&server_tcp;
        conn->proto.tcp->local_port = espconn_port();
        conn->proto.tcp->remote_port = REMOTE_PORT;

        os_memcpy( conn->proto.tcp->remote_ip, &target->addr, 4 );

        espconn_regist_connectcb( conn, tcp_connected );
        espconn_regist_disconcb( conn, tcp_disconnected );
        espconn_regist_reconcb( conn, tcp_error );

        espconn_connect( conn );
}

void wifi_callback( System_Event_t *evt )
{
        os_printf( "%s: %d\n", __FUNCTION__, evt->event );

        switch ( evt->event )
        {
        case EVENT_STAMODE_CONNECTED:
        {
                os_printf("connect to ssid %s, channel %d\n",
                          evt->event_info.connected.ssid,
                          evt->event_info.connected.channel);
                break;
        }

        case EVENT_STAMODE_DISCONNECTED:
        {
                os_printf("disconnect from ssid %s, reason %d\n",
                          evt->event_info.disconnected.ssid,
                          evt->event_info.disconnected.reason);

                goto_sleep();
                break;
        }

        case EVENT_STAMODE_GOT_IP:
        {
                os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
                          IP2STR(&evt->event_info.got_ip.ip),
                          IP2STR(&evt->event_info.got_ip.mask),
                          IP2STR(&evt->event_info.got_ip.gw));
                os_printf("\n");

                #if USE_DNS == true
                  espconn_gethostbyname( &server_conn, REMOTE_HOST, &server_ip, dns_done );
                #else
                  to_ip_address();
                #endif

                break;
        }

        default:
        {
                goto_sleep();
                break;
        }
        }
}


void user_init( void )
{

        static struct station_config config; // this will hold the data

        uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
        os_printf( "%s\n", __FUNCTION__ );

        wifi_station_set_hostname( HOST_NAME );
        wifi_set_opmode_current( STATION_MODE );

        gpio_init();

        config.bssid_set = 0;
        os_memcpy( &config.ssid, SSID, 32 );
        os_memcpy( &config.password, PASSWORD, 64 );
        wifi_station_set_config( &config );
        wifi_set_event_handler_cb( wifi_callback );
}
