/* 
  BLE CUBE - Marco Russi 
*/

/* ---------- Inclusions ---------- */
#include <SoftwareSerial.h>


/* ---------- Defines ---------- */

/* Maximum number of found devices */
#define MAX_NUM_FOUND_DEVICES     6
/* Maximum number of target devices */
#define MAX_NUM_TARGET_DEVICES    3

/* BLE module RX pin */
#define BLE_MODULE_RX_PIN         8  
/* BLE module TX pin */ 
#define BLE_MODULE_TX_PIN         9
/* BLE module role pin - LOW is central, HIGH is peripheral */
#define BLE_MODULE_ROLE_PIN       7   
/* BLE module reset pin - active LOW */ 
#define BLE_MODULE_RESET_PIN      6
/* BLE module connection pin - LOW is disconnected, HIGH is connected */ 
#define BLE_MODULE_CONN_PIN       5
/* BLE module role pin value as peripheral */
#define ROLE_PERIPH_PIN_VALUE     HIGH 
/* BLE module role pin value as central */
#define ROLE_CENTRAL_PIN_VALUE    LOW
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

/* Found devices addresses list */
String found_dev_add[MAX_NUM_FOUND_DEVICES];
/* Found devices names list */
String found_dev_names[MAX_NUM_FOUND_DEVICES];
/* Number of found devices */
int num_found_devices = 0;  /* reset value */

/* Target devices name strings */
String target_devices[MAX_NUM_TARGET_DEVICES] = {
  "ble_led_ctrl_02",
  "ble_led_ctrl_03",
  "ble_led_ctrl_04",
};

/* Devices name strings */
String face_commands[6] = {
  "1:B100-2:B020",
  "1:B030-2:B080",
  "",
  "",
  "",
  ""
};

int dev_index_array[MAX_NUM_TARGET_DEVICES];  // TODO: init it in setup()

bool device_connected[MAX_NUM_TARGET_DEVICES];

boolean ble_module_central = true;




/* ----------- Functions implementation ------------ */

/* Setup function */
void setup() 
{
  int i;
  
  /* set BLE module reset pin as output */
  pinMode(BLE_MODULE_RESET_PIN, OUTPUT);
  /* set BLE module role pin as output */
  pinMode(BLE_MODULE_ROLE_PIN, OUTPUT);
  /* Reset BLE module */
  digitalWrite(BLE_MODULE_RESET_PIN, RESET_ACTIVE_PIN_VALUE);
  /* immediately set the default BLE role */
  digitalWrite(BLE_MODULE_ROLE_PIN, ROLE_CENTRAL_PIN_VALUE);
  /* start BLE module as central */
  if(ble_module_central == true)
  {
    digitalWrite(BLE_MODULE_ROLE_PIN, ROLE_CENTRAL_PIN_VALUE);
  }
  /* or as peripheral */
  else
  {
    digitalWrite(BLE_MODULE_ROLE_PIN, ROLE_PERIPH_PIN_VALUE);
  }
  /* wait for a while (50ms) */
  delay(50);
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


  for(i=0; i<MAX_NUM_TARGET_DEVICES; i++)
  {
    device_connected[i] = false;
  }




  ble_scan_devices();
}


/* Loop function */
void loop() 
{
  
  /* get resp if present */
  //ble_get_resp();

  /* if BLE module is in central role */
  if(ble_module_central == true)
  {
    /* rx and parse serial command */
    rx_and_parse_cmd();
  }
  else
  /* alse in peripheral role */
  {
    /* manage received data */
    if (mySerial.available())
    {
      /* if connection pin indicates an established connection */
      if( CONNECTED_PIN_VALUE == digitalRead(BLE_MODULE_CONN_PIN) )
      {
        Serial.write(mySerial.read());  // TO REMOVE
        /* manage received data from peripheral role */
        manage_periph_data();
      }
      /* else there is no connection */
      else
      {
        /* discard received data */
        mySerial.read();
      }
    }
    else
    {
      /* do nothing */
    }
  }
  
}


/* Function to manage data received from peripheral role */
void manage_periph_data(void)
{
  /* do nothing at the moment */
  /* TODO */
}


/* Function to execute all the preconfigured commands */
void execute_controls(int face_index)
{
  int target_count, i, sep1, sep2;
  int device_index;
  String temp_string = "";
  String command = "";
  char cmd_string[20];
  boolean end_of_cmd = false;
  String temp_commands[MAX_NUM_TARGET_DEVICES];

  /* scan devices */
  //ble_scan_devices();

  /* get command string */
  temp_string += face_commands[face_index];
  /* if command string is not null */
  if( temp_string != "")
  {
    end_of_cmd = false;
    sep2 = 0;
    target_count = 0;
    while(false == end_of_cmd)
    {
      sep1 = temp_string.indexOf(':', sep2);
      device_index = (temp_string[sep1-1] & 0x0F);
      sep2 = temp_string.indexOf('-', sep1);
      /* if last substring */
      if(sep2 == -1)
      {
        /* get last substring */
        command += temp_string.substring(sep1+1);
        end_of_cmd = true;
      }
      else
      {
        /* get substring */
        command += temp_string.substring(sep1+1, sep2);
      }

      Serial.println("=========");
      Serial.println(device_index);
      temp_commands[device_index] += command;
      Serial.println(temp_commands[device_index]);
      command.remove(0);

      if(device_connected[device_index] == false)
      {
        /* try to connect */
        if(true == ble_connect_target_dev(dev_index_array[device_index]))
        {
          device_connected[device_index] = true;
        }
        else
        {
          /* fail to connect */
        }
      }
      else
      {
        /* already connected */
      }
      delay(1000);
      mySerial.write('*');
      while(true != ble_get_resp());
      delay(10);
      /* next target */
      target_count++;
    }
    temp_string.remove(0);


    Serial.print("$$$$$$$$$ - ");
    Serial.println(target_count);
    //for(i=0; i<target_count; i++)
    for(i=0; i<MAX_NUM_TARGET_DEVICES; i++)
    {
      /* if device is connected */
      if(device_connected[i] == true)
      {
        Serial.print("switch");
        sprintf(cmd_string, "AT+SWITCH=%d.", dev_index_array[i]);
        mySerial.print(cmd_string);
        while(true != ble_get_resp());
        delay(100);
        Serial.print("->");
        Serial.println(temp_commands[i]);
        mySerial.print(temp_commands[i]);
        mySerial.write('.');
        while(true != ble_get_resp());
        delay(200);
        mySerial.write('*');
        while(true != ble_get_resp());
      }
      else
      {
        /* do nothing */
      }
    }


    Serial.println("""""""""""");
    //for(i=0; i<target_count; i++)
    for(i=0; i<MAX_NUM_TARGET_DEVICES; i++)
    {
      /* if device is connected */
      if(device_connected[i] == true)
      {
        /* drop connection without escaping, already done above */
        ble_drop_connection(false, dev_index_array[i]);
        Serial.print("drop:");
        Serial.println(dev_index_array[i]);
        /* device is disconnected */
        device_connected[i] = false;
      }
      else
      {
        /* do nothing */
      }
    }
  }
  else
  {
    /* do nothing */
    Serial.println("invalid face data");  // TO REMOVE
  }
}


void rx_and_parse_cmd(void)
{
  String uart_buffer;
  String temp_string;
  int target_dev_index;

  if( Serial.available() > 0 ) 
  {
    /* copy each character into the buffer */
    do {
      char c = Serial.read();
      uart_buffer += c;
    } while (Serial.available());
    
    /* parse received command */
    if(uart_buffer.startsWith("FACE:"))
    {
      /* get required face index */
      int pos = uart_buffer.indexOf(':');
      /* consider one character only as index */
      temp_string += uart_buffer[pos+1];
      /* convert it into an integer */
      target_dev_index = temp_string.toInt(); 
      /* connect to the required target device */
      Serial.print("face:"); // TO REMOVE
      Serial.println(target_dev_index); // TO REMOVE
      execute_controls(target_dev_index);
    }
    else
    {
      /* invalid command; do nothing */
      Serial.println("CRAP"); // TO REMOVE
    }
  }
  else
  {
    /* no data received */
  }
}

int temp_dev_index = 0; // TODO: temporary solution
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
  for(temp_dev_index=0; temp_dev_index<temp_max_num_dev; temp_dev_index++)
  {
    /* prepare command string to get found device info */
    sprintf(cmd_string, "AT+FOUND=%d.", temp_dev_index);
    mySerial.print(cmd_string);
    while(true != ble_get_resp());
  }


  for(i=0; i<MAX_NUM_TARGET_DEVICES; i++)
  {
    temp_dev_index = 0;
    while((found_dev_names[temp_dev_index] != target_devices[i]) 
    &&    (temp_dev_index < num_found_devices))
    {
      temp_dev_index++;
    }
  
    /* if device is not found */
    if(temp_dev_index >= num_found_devices)
    {
      /* not found, set 0xFF */
      dev_index_array[i] = 0xFF;
    }
    else
    {
      /* device found, valid index */
      dev_index_array[i] = temp_dev_index;
    }

    Serial.print("->"); 
    Serial.println(dev_index_array[i]); // TO REMOVE
  }
}


/* Function to connect to a target device */
// TODO: check connection index validity
void ble_drop_connection(bool escape, int conn_index)
{
  char cmd_string[20];

  /* send escape string if required */
  if(escape == true)
  {
    /* send escape string */
    mySerial.write('*');
    /* wait for response */
    while(true != ble_get_resp());
  }
  else
  {
    /* do not escape. It should be done before calling this function */
  }
  /* send drop string */
  sprintf(cmd_string, "AT+DROP=%d.", conn_index);
  mySerial.print(cmd_string);
  while(true != ble_get_resp());
}


/* Function to connect to a target device */
bool ble_connect_target_dev(int device_index)
{
  char cmd_string[20];
  int found_dev_index;
  bool success;

  /* get found device index from required target device index */
  //found_dev_index = get_found_dev_index(target_dev_index);
  /* check device index validity */
  //if(found_dev_index != 0xFF)
  if(device_index < num_found_devices)
  {
    /* prepare connection command string */
    sprintf(cmd_string, "AT+CONN=%d.", device_index);
    /* send command string */
    mySerial.print(cmd_string);
    Serial.println(cmd_string); // TO REMOVE
    while(true != ble_get_resp());
    Serial.println("CONNESSO"); // TO REMOVE
    success = true;
  }
  else
  {
    /* device is not found; do nothing */
    Serial.println("DEVICE NOT FOUND");
    success = false;
  }

  return success;
}


/* Function to get found device index */
int get_found_dev_index(int target_dev_index)
{
  int dev_index = 0;

  /* seek previously found device index corresponding to the required target device */
  while((found_dev_names[dev_index] != target_devices[target_dev_index]) 
  &&    (dev_index < num_found_devices))
  {
    dev_index++;
  }

  /* if device is not found */
  if(dev_index >= num_found_devices)
  {
    /* return a "not found" code */
    dev_index = 0xFF;
  }
  else
  {
    /* device found, valid index */
  }
  
  return dev_index;
}


/* get BLE response */
/* ATTENTION: response validity according to previous command is not checked yet */
/* TODO: improve response mechanism */
boolean ble_get_resp(void)
{
  int resp_length = 0;
  int sep;
  int dev_index;
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
      else if (ble_resp_buffer.startsWith("CONNECTED")) 
      {
        /* CONNECTED received */
        /* valid response */
        success = true;
        Serial.println("connected");// TO REMOVE
      }
      else if (ble_resp_buffer.startsWith("OK-")) 
      {
        temp_string += ble_resp_buffer.substring(3);
        if(resp_length > 5)
        {
          /* get device address */
          sep = temp_string.indexOf('-');
          found_dev_add[temp_dev_index] += temp_string.substring(0, sep);
          temp_string.remove(0, sep+1);
          Serial.println(found_dev_add[temp_dev_index]);
          /* get device name */
          sep = temp_string.indexOf('.');
          found_dev_names[temp_dev_index] += temp_string.substring(0, sep);
          temp_string.remove(0, sep+1);
          Serial.println(found_dev_names[temp_dev_index]);
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
      else if (ble_resp_buffer.startsWith("SENT")) 
      {
        /* valid response */
        success = true;
        Serial.println("sent");// TO REMOVE
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




