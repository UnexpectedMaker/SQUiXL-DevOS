#include "Arduino.h"
#include "LittleFS.h"
#include "FS.h"

// List of commands accepted
enum
{
	CMD_NOP = 0,
	CMD_LIST,
	CMD_FORMAT,
	CMD_READ,
	CMD_WRITE,
	CMD_DELETE,
	CMD_STATUS,
	CMD_NAME,
	CMD_COUNT
};

// status/errors
enum
{
	STATUS_OK = 0,
	STATUS_ERROR,
	STATUS_FILE_NOT_FOUND,
	STATUS_FILE_EXISTS,
	STATUS_MOUNT_ERROR
};

char szName[64];
uint8_t u8Data[64];
int i, iTotal, iDataLen;
File file, root;
char *pListing;
bool littlefs_ready = false;

void littlefs_cli()
{
	if (Serial.available())
	{							// wait for commands to arrive
		Serial.read(u8Data, 4); // read command and data length
		if (u8Data[0] > CMD_NOP && u8Data[0] < CMD_COUNT)
		{																 // valid command
			iDataLen = u8Data[1] + (u8Data[2] << 8) + (u8Data[3] << 16); // payload length?
#ifdef DISPLAY_STATUS
			lcd.println(szCmdNames[u8Data[0]]);
#endif
			switch (u8Data[0])
			{
			case CMD_LIST:
				pListing = (char *)malloc(4096); // DEBUG - should be enough
				iDataLen = 0;
				root = LittleFS.open("/");
				file = root.openNextFile();
				while (file)
				{
					if (!file.isDirectory() && file.size() > 0)
					{
						sprintf(szName, "%s - %d bytes\n", file.name(), file.size());
						strcpy(&pListing[iDataLen], szName);
						iDataLen += strlen(szName);
					}
					file = root.openNextFile();
				}
				root.close();
				szName[iDataLen] = 0;			 // zero terminate it as one long string
				u8Data[0] = STATUS_OK;			 // tell the PC to send the data
				u8Data[1] = (uint8_t)(iDataLen); // tell the receiver the length of the text
				u8Data[2] = (uint8_t)(iDataLen >> 8);
				u8Data[3] = 0;
				Serial.write(u8Data, 4);
				iTotal = 0;
				while (iDataLen)
				{
					i = iDataLen;
					if (i > 64)
						i = 64; // max HID payload size
					Serial.write(&pListing[iTotal], i);
					iDataLen -= i;
					iTotal += i;
				}
				free(pListing);
				break;
			case CMD_STATUS:
				u8Data[0] = (littlefs_ready) ? STATUS_OK : STATUS_MOUNT_ERROR;
				if (littlefs_ready)
				{ // return the free space too
					uint32_t u32FreeSpace;
					u32FreeSpace = (uint32_t)(LittleFS.totalBytes() - LittleFS.usedBytes());
					u8Data[1] = (uint8_t)(u32FreeSpace);
					u8Data[2] = (uint8_t)(u32FreeSpace >> 8);
					u8Data[3] = (uint8_t)(u32FreeSpace >> 16);
				}
				Serial.write(u8Data, 4);
				break;
			case CMD_NAME:							// receive the name
				szName[0] = '/';					// always write to root
				Serial.read(&szName[1], u8Data[1]); // read the filename
				szName[u8Data[1] + 1] = 0;			// zero terminate it
				break;
			case CMD_READ:
				if (!LittleFS.exists(szName))
				{
					u8Data[0] = STATUS_FILE_NOT_FOUND;
					Serial.write(u8Data, 4);
				}
				else
				{
					file = LittleFS.open(szName, FILE_READ);
					u8Data[0] = STATUS_OK;
					iDataLen = file.size();
					u8Data[1] = (uint8_t)(iDataLen);
					u8Data[2] = (uint8_t)(iDataLen >> 8);
					u8Data[3] = (uint8_t)(iDataLen >> 16);
					Serial.write(u8Data, 4); // tell the PC the file size
					while (iDataLen)
					{
						i = iDataLen;
						if (i > 64)
							i = 64;			  // max HID payload size
						file.read(u8Data, i); // this could take a long time
						Serial.write(u8Data, i);
						iDataLen -= i;
					} // while sending the data
					file.close();
				} // file exists
				break;
			case CMD_WRITE:
				if (LittleFS.exists(szName))
				{
					u8Data[0] = STATUS_FILE_EXISTS;
					Serial.write(u8Data, 4);
				}
				else
				{
					file = LittleFS.open(szName, FILE_WRITE);
					if (!file)
					{
						u8Data[0] = STATUS_ERROR;
						Serial.write(u8Data, 4);
					}
					else
					{
						u8Data[0] = STATUS_OK; // tell the PC to send the data
						Serial.write(u8Data, 4);
						iTotal = 0;
						while (iDataLen)
						{
							while (!Serial.available())
							{
								vTaskDelay(1); // wait for data to arrive
							}
							i = iDataLen;
							if (i > 64)
								i = 64; // max HID payload size
							Serial.read(u8Data, i);
							iTotal += file.write(u8Data, i); // this could take a long time
							// The PC side could write and we might miss it, so we'll do a back
							// and forth protocol to ensure the data doesn't get corrupted
							Serial.write('A'); // write 1 byte heartbeat
							iDataLen -= i;
						}
						file.flush();
						file.close();
						u8Data[0] = STATUS_OK;
						Serial.write(u8Data, 4); // send a regular status block
					}
				}
				break;
			case CMD_DELETE:
				if (LittleFS.exists(szName))
				{
					if (LittleFS.remove(szName))
					{
						u8Data[0] = STATUS_OK;
					}
					else
					{
						u8Data[0] = STATUS_ERROR;
					}
					Serial.write(u8Data, 4);
				}
				else
				{
					u8Data[0] = STATUS_FILE_NOT_FOUND;
					Serial.write(u8Data, 4);
				}
				break;
			case CMD_FORMAT:
				if (!littlefs_ready)
				{
					littlefs_ready = LittleFS.begin(true);
				}
				else
				{
					littlefs_ready = LittleFS.format();
				}
				u8Data[0] = (littlefs_ready) ? STATUS_OK : STATUS_ERROR; // let PC know when we are finished
				Serial.write(u8Data, 4);
				break;
			} // switch on cmd
		}
	}
	else
	{
		vTaskDelay(1);
	}
} // while (1)