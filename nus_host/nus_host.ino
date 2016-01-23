/* 
  Arduino BLE NUS central host - Marco Russi 
*/

/* ---------- Inclusions ---------- */
#include <SoftwareSerial.h>


/* ---------- Defines ---------- */

/* Maximum number of found devices */
#define MAX_NUM_FOUND_DEVICES     6

/* BLE module RX pin */
#define BLE_MODULE_RX_PIN         8  
/* BLE module TX pin */ 
#define BLE_MODULE_TX_PIN         9
/* BLE module reset pin - active LOW */ 
#define BLE_MODULE_RESET_PIN      6
/* BLE module connection pin - LOW is disconnected, HIGH is connected */ 
#define BLE_MODULE_CONN_PIN       5
/* BLE module reset pin value as active */
#define RESET_ACTIVE_PIN_VALUE    LOW 
/* BLE module reset pin value as not active (released) */
#define RESET_RELEASED_PIN_VALUE  HIGH
/* BLE module connection pin value as connected */
#define CONNECTED_PIN_VALUE       HIGH 
/* BLE module connection pin value as disconnected */
#define DISCONNECTED_PIN_VALUE    LOW


/* ---------- Local variables ---------- */

/* Software serial */
SoftwareSerial mySerial(BLE_MODULE_RX_PIN, BLE_MODULE_TX_PIN);  /* RX, TX */

/* Number of found devices */
int num_found_devices = 0;  /* reset value */

bool device_connected[MAX_NUM_FOUND_DEVICES];

static volatile bool data_mode = false;




/* ----------- Functions implementation ------------ */

/* Setup function */
void setup() 
{
  int i;
  
  /* set BLE module reset pin as output */
  pinMode(BLE_MODULE_RESET_PIN, OUTPUT);
  /* Reset BLE module */
  digitalWrite(BLE_MODULE_RESET_PIN, RESET_ACTIVE_PIN_VALUE);
  /* wait for a while (100ms) */
  delay(100);
  /* Release BLE module reset - start BLE module */
  digitalWrite(BLE_MODULE_RESET_PIN, RESET_RELEASED_PIN_VALUE);

  /* ATTENTION: serial init must be performed after all I/O pins stuff */
  /* Open serial communications and wait for port to open */
  Serial.begin(38400);
  /* wait for serial port to connect. Needed for native USB port only */
  while (!Serial);
  Serial.println("Eccomi!");
  /* set the data rate for the SoftwareSerial port */
  mySerial.begin(38400);

  /* clear connection flags */
  for(i=0; i<MAX_NUM_FOUND_DEVICES; i++)
  {
    device_connected[i] = false;
  }
}


/* Loop function */
void loop() 
{
  String uart_buffer;
  
  if( Serial.available() > 0 ) 
  {
    /* copy each character into the buffer */
    do {
      char c = Serial.read();
      uart_buffer += c;
    } while (Serial.available());
  
    /* data mode */
    if(data_mode == true)
    {
      /* escape sequence */
      if(uart_buffer.startsWith("***"))
      {
        delay(50);
        mySerial.write('*');
        while(true != ble_get_resp());
        delay(50);
        /* data mode disabled */
        data_mode = false;
      }
      else
      {
        /* send data */
        mySerial.print(uart_buffer);
        /* consume eventual data response from module */
        while(true != ble_get_resp());
      }
    }
    /* configuration mode */
    else
    {
      /* rx and parse serial command */
      rx_and_parse_cmd(uart_buffer);
    }
  }
  else
  {
    /* do nothing */
  }
}


void rx_and_parse_cmd(String uart_buffer)
{
  String temp_string;
  int dev_index;
  char cmd_string[20];

    /* parse received command */
    /* CONN:x */
    if(uart_buffer.startsWith("CONN:"))
    {
      /* get required connection index */
      int pos = uart_buffer.indexOf(':');
      /* consider one character only as index */
      temp_string += uart_buffer[pos+1];
      /* convert it into an integer */
      dev_index = temp_string.toInt();
      /* if device is connected */
      if((device_connected[dev_index] == false)
      && (dev_index < num_found_devices)
      && (num_found_devices != 0))
      {
        Serial.print("conn:"); // TO REMOVE
        Serial.println(dev_index); // TO REMOVE
        /* prepare connection command string */
        sprintf(cmd_string, "AT+CONN=%d.", dev_index);
        /* send command string */
        mySerial.print(cmd_string);
        Serial.println(cmd_string); // TO REMOVE
        while(true != ble_get_resp());
        /* device is connected */
        device_connected[dev_index] = true;
        /* data mode enabled */
        data_mode = true;
      }
      else
      {
        /* already connected */
        Serial.println("error");
      }
    }
    /* SWITCH:x */
    else if(uart_buffer.startsWith("SWITCH:"))
    {
      /* get required connection index */
      int pos = uart_buffer.indexOf(':');
      /* consider one character only as index */
      temp_string += uart_buffer[pos+1];
      /* convert it into an integer */
      dev_index = temp_string.toInt(); 
      /* if device is connected */
      if((device_connected[dev_index] == true)
      && (dev_index < num_found_devices)
      && (num_found_devices != 0))
      {
        Serial.print("switch");
        sprintf(cmd_string, "AT+SWITCH=%d.", dev_index);
        mySerial.print(cmd_string);
        while(true != ble_get_resp());
        delay(100);
        /* data mode enabled */
        data_mode = true;
      }
      else
      {
        /* do nothing */
        Serial.println("error");
      }
    }
    /* DROP:x */
    else if(uart_buffer.startsWith("DROP:"))
    {
      /* drop connection */
      /* get required connection index */
      int pos = uart_buffer.indexOf(':');
      /* consider one character only as index */
      temp_string += uart_buffer[pos+1];
      /* convert it into an integer */
      dev_index = temp_string.toInt(); 
      /* if device is connected */
      if((device_connected[dev_index] == true)
      && (dev_index < num_found_devices)
      && (num_found_devices != 0))
      {
        /* drop connection without escaping, already done above */
        Serial.print("drop:");
        Serial.println(dev_index);
        /* send drop string */
        sprintf(cmd_string, "AT+DROP=%d.", dev_index);
        mySerial.print(cmd_string);
        while(true != ble_get_resp());
        /* device is disconnected */
        device_connected[dev_index] = false;
      }
      else
      {
        /* do nothing */
        Serial.println("error");
      }
    }
    /* AUTO */
    else if(uart_buffer.startsWith("AUTO"))
    {
      mySerial.print("AT+AUTO");

      /* data mode enabled */
      data_mode = true;
    }
    /* SCAN */
    else if(uart_buffer.startsWith("SCAN"))
    {
      /* scan devices */
      ble_scan_devices();
    }
    else
    {
      /* invalid command; do nothing */
      Serial.println("WRONG");
    }

}


/* Function to scan BLE devices */
void ble_scan_devices(void)
{
  char cmd_string[20];
  int temp_max_num_dev;
  int i;
  
  /* send scan start string */
  mySerial.print("AT+SCAN+.");
  while(true != ble_get_resp());
  /* wait enough time... */
  delay(3000);
  /* send scan stop string */
  mySerial.print("AT+SCAN-.");
  while(true != ble_get_resp());

  /* limit number of found devices */
  if(num_found_devices > MAX_NUM_FOUND_DEVICES)
  {
    temp_max_num_dev = MAX_NUM_FOUND_DEVICES;
  }
  else
  {
    temp_max_num_dev = num_found_devices;
  }

  /* get found devices info */
  for(i=0; i<temp_max_num_dev; i++)
  {
    /* prepare command string to get found device info */
    sprintf(cmd_string, "AT+FOUND=%d.", i);
    mySerial.print(cmd_string);
    while(true != ble_get_resp());
  }
}


/* get BLE response */
/* ATTENTION: response validity according to previous command is not checked yet */
/* TODO: improve response mechanism */
boolean ble_get_resp(void)
{
  int resp_length = 0;
  int sep;
  String ble_resp_buffer;
  String temp_string = "";
  /* init as invalid response */
  boolean success = false;
  
  if( mySerial.available() > 0 ) 
  {
    char c;
    /* copy each character into the data string */
    do {
      do {
        c = mySerial.read();
        ble_resp_buffer += c;
        resp_length++;
      } while(mySerial.available());
    } while(c != '.');
    
    /* print data for debugging */
    Serial.print("---");
    Serial.println(ble_resp_buffer);
    //Serial.println("--------");

    /* length is 1 at least */
    /* parse the received response */
    /* consider length greater than 1 only */
    if(resp_length > 1)
    {
      if (ble_resp_buffer.startsWith("CHE")) 
      {
        /* CHE received */
        /* valid response */
        success = true;
        Serial.println("che");// TO REMOVE
      }
      else if (ble_resp_buffer.startsWith("WAIT")) 
      {
        /* WAIT received */
        /* consider as not valid in order to wait the next result response */
        Serial.println("wait");// TO REMOVE
      }
      else if (ble_resp_buffer.startsWith("OK-")) 
      {
        temp_string += ble_resp_buffer.substring(3);
        if(resp_length > 5)
        {
          /* get device address */
          sep = temp_string.indexOf('-');
          temp_string.remove(0, sep+1);
          Serial.println(temp_string.substring(0, sep));
          /* get device name */
          sep = temp_string.indexOf('.');
          temp_string.remove(0, sep+1);
          Serial.println(temp_string.substring(0, sep));
        }
        else
        {
          /* get number of found devices */
          num_found_devices = temp_string.toInt();
          Serial.println(num_found_devices);
        }

        /* valid response */
        success = true;
      }
      else if (ble_resp_buffer.startsWith("OK")) 
      {
        /* positive ACK received */
        Serial.println("ok");// TO REMOVE
        /* valid response */
        success = true;
      }
      /* any other responses */
      else
      {
        /* do nothing */
        Serial.println("BO3");// TO REMOVE
        /* ATTENTION: set as success at the moment */
        success = true;
      }
    }
    else
    {
      /* response is 1 byte long; invalid; do nothing */
      Serial.println("BO2");// TO REMOVE
      /* ATTENTION: set as success at the moment */
      //success = true;
    }
    
    /* clean buffer */
    ble_resp_buffer.remove(0);
  }
  else
  {
    /* response not received; do nothing */
    //Serial.println("BO1");// TO REMOVE
  }

  return success;
}




/* End of file */




