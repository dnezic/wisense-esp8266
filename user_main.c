#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"
#include "bme280.h"
//#include "lwip/inet.h"


struct espconn dweet_conn;
ip_addr_t dweet_ip;
esp_tcp dweet_tcp;

char dweet_host[] = "192.168.0.11";
char dweet_path[] = "/data";
char json_data[ 256 ];
char dt[ 32 ];
char dp[ 32 ];
char dh[ 32 ];
char buffer[ 2048 ];

LOCAL int ICACHE_FLASH_ATTR power(int base, int exp){
    int result = 1;
    while(exp) { result *= base; exp--; }
    return result;
}

LOCAL char* ICACHE_FLASH_ATTR f2s(float num, uint8_t decimals) {
  static char* buf[16];
  int whole = num;
  int decimal = (num - whole) * power(10, decimals);
  if (decimal < 0) {
    // get rid of sign on decimal portion
    decimal -= 2 * decimal;
  }
  char* pattern[10]; // setup printf pattern for decimal portion
  os_sprintf(pattern, "%%d.%%0%dd", decimals);
  os_sprintf(buf, pattern, whole, decimal);
  return (char *)buf;
}




void user_rf_pre_init( void )
{
}


void goto_sleep() {

deep_sleep_set_option( 0 );
system_deep_sleep( 60 * 1000 * 1000 );  // 60 seconds


}



void data_received( void *arg, char *pdata, unsigned short len )
{
    struct espconn *conn = arg;

    os_printf( "%s: %s\n", __FUNCTION__, pdata );

    espconn_disconnect( conn );
}


void tcp_connected( void *arg )
{
    int temperature = 55;   // test data

    struct sensor_result sr;
    read_all(&sr);
    os_sprintf( dt, "%s\n", f2s(sr.temperature,2));
    os_sprintf( dh, "%s\n", f2s(sr.humidity,2));
    os_sprintf( dp, "%s\n", f2s(sr.pressure,2));
    // user_mvh3004_read_th(humiture_data);

    struct espconn *conn = arg;

    os_printf( "%s\n", __FUNCTION__ );
    espconn_regist_recvcb( conn, data_received );

    os_sprintf( json_data, "{\"temperature\": \"%s\", \"humidity\": \"%s\", \"pressure\": \"%s\"}", dt, dh, dp );
    os_sprintf( buffer, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
                         dweet_path, dweet_host, os_strlen( json_data ), json_data );

    os_printf( "Sending: %s\n", buffer );
    sint8 err;
    err = espconn_sent( conn, buffer, os_strlen( buffer ) );
    os_printf("Sent: %d\n", err);
    if(err != 0) {
     goto_sleep();
    }
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
    goto_sleep();
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
        conn->proto.tcp=&dweet_tcp;
        conn->proto.tcp->local_port = espconn_port();
        conn->proto.tcp->remote_port = 80;
        os_memcpy( conn->proto.tcp->remote_ip, &ipaddr->addr, 4 );

        espconn_regist_connectcb( conn, tcp_connected );
        espconn_regist_disconcb( conn, tcp_disconnected );

        espconn_connect( conn );
    }
}

void send_data( )
{
    struct espconn *conn = &dweet_conn;
    ipaddr_aton("192.168.0.11", &dweet_ip);
    ip_addr_t *target = &dweet_ip;


    os_printf( "%s\n", __FUNCTION__ );

    os_printf("Connecting...\n" );

    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp=&dweet_tcp;
    conn->proto.tcp->local_port = espconn_port();
    conn->proto.tcp->remote_port = 80;

    os_printf("test1\n");

    os_memcpy( conn->proto.tcp->remote_ip, &target->addr, 4 );

   os_printf("test2\n" );


    espconn_regist_connectcb( conn, tcp_connected );
    espconn_regist_disconcb( conn, tcp_disconnected );
    espconn_regist_reconcb( conn, tcp_error );
os_printf("test3...%d\n", target->addr );

    sint8 err = espconn_connect( conn );
    os_printf("State: %d\n", err);
    if(err != 0) {
        goto_sleep();
    }

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

            // espconn_gethostbyname( &dweet_conn, dweet_host, &dweet_ip, dns_done );
            send_data();

            break;
        }

        default:
        {
            break;
        }
    }
}


void user_init( void )
{
    static struct station_config config;

    uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
    os_printf( "%s\n", __FUNCTION__ );

    wifi_station_set_hostname( "svecomp111" );
    wifi_set_opmode_current( STATION_MODE );

    gpio_init();

    config.bssid_set = 0;
    os_memcpy( &config.ssid, "svesoftware", 32 );
    os_memcpy( &config.password, "pass", 64 );
    wifi_station_set_config( &config );

    wifi_set_event_handler_cb( wifi_callback );
}
