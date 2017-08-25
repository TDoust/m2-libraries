/****************************************************************************************************

PowerDueFirmware - SD Mass storage
Adapted from:
RepRapFirmware - Platform: RepRapPro Ormerod with Duet controller
-----------------------------------------------------------------------------------------------------

Version 0.0.1
26 August 2016
Joao Diogo Falcao
Licence: GPL

Modified for use with Macchina M2 Tony Doust

****************************************************************************************************/

#include "M2_Due_SD_HSCMI.h"

/*
 * Debug Functions (Change output channel if needed)
 */
void Debug(const char* header, const char* msg){
  if(SerialUSB && SD_DEBUG){
    SerialUSB.print(header);
    SerialUSB.print(": ");
    SerialUSB.print(msg);
  }
}

void Debug(const char* header, unsigned char msg){
  if(SerialUSB && SD_DEBUG){
    SerialUSB.print(header);
    SerialUSB.print(": ");
    SerialUSB.print(msg);
  }
}

void Debug(unsigned char msg){
  if(SerialUSB && SD_DEBUG){
    SerialUSB.print(msg);
  }
}

void Debug(const char* msg){
  if(SerialUSB && SD_DEBUG){
    SerialUSB.print(msg);
  }
}


bool StringEquals(const char* s1, const char* s2)
{
 int i = 0;
 while(s1[i] && s2[i])
 {
   if(tolower(s1[i]) != tolower(s2[i]))
   {
     return false;
   }
   i++;
 }

 return !(s1[i] || s2[i]);
}

/*********************************************************************************

 Files & Communication

 */

MassStorage::MassStorage() : combinedName(combinedNameBuff, ARRAY_SIZE(combinedNameBuff))
{
	memset(&fileSystem, 0, sizeof(FATFS));
	findDir = new DIR();
}

bool MassStorage::Init()
{
	// Initialize SD MMC stack

  // Initialize HSMCI pins for the M2
	PIO_Configure(g_APinDescription[PIN_HSMCI_MCCDA_GPIO].pPort, g_APinDescription[PIN_HSMCI_MCCDA_GPIO].ulPinType, g_APinDescription[PIN_HSMCI_MCCDA_GPIO].ulPin, g_APinDescription[PIN_HSMCI_MCCDA_GPIO].ulPinConfiguration);
	PIO_Configure(g_APinDescription[PIN_HSMCI_MCCK_GPIO].pPort, g_APinDescription[PIN_HSMCI_MCCK_GPIO].ulPinType, g_APinDescription[PIN_HSMCI_MCCK_GPIO].ulPin, g_APinDescription[PIN_HSMCI_MCCK_GPIO].ulPinConfiguration);
	PIO_Configure(g_APinDescription[PIN_HSMCI_MCDA0_GPIO].pPort, g_APinDescription[PIN_HSMCI_MCDA0_GPIO].ulPinType, g_APinDescription[PIN_HSMCI_MCDA0_GPIO].ulPin, g_APinDescription[PIN_HSMCI_MCDA0_GPIO].ulPinConfiguration);
	PIO_Configure(g_APinDescription[PIN_HSMCI_MCDA1_GPIO].pPort, g_APinDescription[PIN_HSMCI_MCDA1_GPIO].ulPinType, g_APinDescription[PIN_HSMCI_MCDA1_GPIO].ulPin, g_APinDescription[PIN_HSMCI_MCDA1_GPIO].ulPinConfiguration);
	PIO_Configure(g_APinDescription[PIN_HSMCI_MCDA2_GPIO].pPort, g_APinDescription[PIN_HSMCI_MCDA2_GPIO].ulPinType, g_APinDescription[PIN_HSMCI_MCDA2_GPIO].ulPin, g_APinDescription[PIN_HSMCI_MCDA2_GPIO].ulPinConfiguration);
	PIO_Configure(g_APinDescription[PIN_HSMCI_MCDA3_GPIO].pPort, g_APinDescription[PIN_HSMCI_MCDA3_GPIO].ulPinType, g_APinDescription[PIN_HSMCI_MCDA3_GPIO].ulPin, g_APinDescription[PIN_HSMCI_MCDA3_GPIO].ulPinConfiguration);

	//set pullups (not on clock!)
  digitalWrite(PIN_HSMCI_MCCDA_GPIO_ARDUINO, HIGH);
  digitalWrite(PIN_HSMCI_MCDA0_GPIO_ARDUINO, HIGH);
  digitalWrite(PIN_HSMCI_MCDA1_GPIO_ARDUINO, HIGH);
  digitalWrite(PIN_HSMCI_MCDA2_GPIO_ARDUINO, HIGH);
  digitalWrite(PIN_HSMCI_MCDA3_GPIO_ARDUINO, HIGH);
  pinMode(PIN_HSMCI_CARD_DETECT_ARDUINO, INPUT);


	sd_mmc_init();
	delay(20);

	bool abort = false;
	sd_mmc_err_t err;
  unsigned int now = millis();
	do {
		err = sd_mmc_check(0);
		if (err > SD_MMC_ERR_NO_CARD)
		{
			abort = true;
			delay(3000);	// Wait a few seconds, so users have a chance to see the following error message
		}
		else
		{
			abort = (err == SD_MMC_ERR_NO_CARD && (millis()-now) > 5000);
		}

		if (abort)
		{
      Debug("MS","Cannot initialize the SD card: ");

			switch (err)
			{
				case SD_MMC_ERR_NO_CARD:
					Debug("MS", "Card not found\n");
					break;
				case SD_MMC_ERR_UNUSABLE:
					Debug("MS", "Card is unusable, try another one\n");
					break;
				case SD_MMC_ERR_SLOT:
					Debug("MS", "Slot unknown\n");
					break;
				case SD_MMC_ERR_COMM:
					Debug("MS","General communication error\n");
					break;
				case SD_MMC_ERR_PARAM:
					Debug("MS","Illegal input parameter\n");
					break;
				case SD_MMC_ERR_WP:
					Debug("MS","Card write protected\n");
					break;
				default:
					Debug("MS", "Unknown (code ");
					Debug("MS",err);
					Debug("MS",")\n");
					break;
			}
			return false;
		}
	} while (err != SD_MMC_OK);

	// Print some card details (optional)

	Debug("MS", "SD card detected!\nCapacity: ");
  Debug(sd_mmc_get_capacity(0));
  Debug("\n");
	Debug("MS", "Bus clock: ");
  Debug(sd_mmc_get_bus_clock(0));
  Debug("\n");
	Debug("MS", "Bus width: ");
  Debug(sd_mmc_get_bus_width(0));
  Debug("\n");
  Debug("MS", "Card type: ");
	switch (sd_mmc_get_type(0))
	{
		case CARD_TYPE_SD | CARD_TYPE_HC:
			Debug("SDHC\n");
			break;
		case CARD_TYPE_SD:
			Debug("SD\n");
			break;
		case CARD_TYPE_MMC | CARD_TYPE_HC:
			Debug("MMC High Density\n");
			break;
		case CARD_TYPE_MMC:
			Debug("MMC\n");
			break;
		case CARD_TYPE_SDIO:
			Debug("SDIO\n");
			return false;
		case CARD_TYPE_SD_COMBO:
			Debug("SD COMBO\n");
			break;
		case CARD_TYPE_UNKNOWN:
		default:
			Debug("Unknown\n");
			return false;
	}

	// Mount the file system

	int mounted = f_mount(0, &fileSystem);
	if (mounted != FR_OK)
	{
		Debug("MS","Can't mount filesystem 0: code ");
		Debug(mounted);
        return false;
	}
    return true;
}


const char* MassStorage::CombineName(const char* directory, const char* fileName)
{
	uint out = 0;
	int in = 0;

	if (directory != NULL)
	{
		while (directory[in] != 0 && directory[in] != '\n')
		{
			combinedName[out] = directory[in];
			in++;
			out++;
			if (out >= combinedName.Length())
			{
				Debug("MS","CombineName() buffer overflow.");
				out = 0;
			}
		}
	}

	if (in > 0 && directory[in -1] != '/' && out < STRING_LENGTH -1)
	{
		combinedName[out] = '/';
		out++;
	}

	in = 0;
	while (fileName[in] != 0 && fileName[in] != '\n')
	{
		combinedName[out] = fileName[in];
		in++;
		out++;
		if (out >= combinedName.Length())
		{
			Debug("MS","CombineName() buffer overflow.");
			out = 0;
		}
	}
	combinedName[out] = 0;

	return combinedName.Pointer();
}

// Open a directory to read a file list. Returns true if it contains any files, false otherwise.
bool MassStorage::FindFirst(const char *directory, FileInfo &file_info)
{
	TCHAR loc[FILENAME_LENGTH];

	// Remove the trailing '/' from the directory name
	size_t len = strnlen(directory, ARRAY_UPB(loc));
	if (len == 0)
	{
		loc[0] = 0;
	}
	else if (directory[len - 1] == '/')
	{
		strncpy(loc, directory, len - 1);
		loc[len - 1] = 0;
	}
	else
	{
		strncpy(loc, directory, len);
		loc[len] = 0;
	}

	findDir->lfn = nullptr;
	FRESULT res = f_opendir(findDir, loc);
	if (res == FR_OK)
	{
		FILINFO entry;
		entry.lfname = file_info.fileName;
		entry.lfsize = ARRAY_SIZE(file_info.fileName);

		for(;;)
		{
			res = f_readdir(findDir, &entry);
			if (res != FR_OK || entry.fname[0] == 0) break;
			if (StringEquals(entry.fname, ".") || StringEquals(entry.fname, "..")) continue;

			file_info.isDirectory = (entry.fattrib & AM_DIR);
			file_info.size = entry.fsize;
			uint16_t day = entry.fdate & 0x1F;
			if (day == 0)
			{
				// This can happen if a transfer hasn't been processed completely.
				day = 1;
			}
			file_info.day = day;
			file_info.month = (entry.fdate & 0x01E0) >> 5;
			file_info.year = (entry.fdate >> 9) + 1980;
			if (file_info.fileName[0] == 0)
			{
				strncpy(file_info.fileName, entry.fname, ARRAY_SIZE(file_info.fileName));
			}

			return true;
		}
	}

	return false;
}

// Find the next file in a directory. Returns true if another file has been read.
bool MassStorage::FindNext(FileInfo &file_info)
{
	FILINFO entry;
	entry.lfname = file_info.fileName;
	entry.lfsize = ARRAY_SIZE(file_info.fileName);

	findDir->lfn = nullptr;
	if (f_readdir(findDir, &entry) != FR_OK || entry.fname[0] == 0)
	{
		//f_closedir(findDir);
		Debug("no file found\n");
		return false;
	}
	Debug("file found\n");
	file_info.isDirectory = (entry.fattrib & AM_DIR);
	Debug("Directory: ",file_info.isDirectory);
	file_info.size = entry.fsize;
	Debug("Size: ",file_info.size);
	uint16_t day = entry.fdate & 0x1F;
	if (day == 0)
	{
		// This can happen if a transfer hasn't been processed completely.
		day = 1;
	}
	file_info.day = day;
	Debug("Day: ",file_info.day);
	file_info.month = (entry.fdate & 0x01E0) >> 5;
	Debug("Month: ",file_info.month);
	file_info.year = (entry.fdate >> 9) + 1980;
	Debug("Year: ",file_info.year);
	if (file_info.fileName[0] == 0)
	{
		strncpy(file_info.fileName, entry.fname, ARRAY_SIZE(file_info.fileName));
	}
	Debug("File name: ",file_info.fileName);
	
	return true;
}

// Month names. The first entry is used for invalid month numbers.
static const char *monthNames[13] = { "???", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// Returns the name of the specified month or '???' if the specified value is invalid.
const char* MassStorage::GetMonthName(const uint8_t month)
{
	return (month <= 12) ? monthNames[month] : monthNames[0];
}

// Delete a file or directory
bool MassStorage::Delete(const char* directory, const char* fileName)
{
	const char* location = (directory != NULL)
							? SD.CombineName(directory, fileName)
								: fileName;
	if (f_unlink(location) != FR_OK)
	{
		Debug("MS","Can't delete file ");
		Debug("MS",location);
    Debug("MS","\n");
		return false;
	}
	Debug("MS","File and directory were deleted.\n");
	return true;
}

// Create a new directory
bool MassStorage::MakeDirectory(const char *parentDir, const char *dirName)
{
	const char* location = SD.CombineName(parentDir, dirName);
	if (f_mkdir(location) != FR_OK)
	{
		Debug("MS","Can't create directory ");
		Debug("MS",location);
		Debug("\n");
		return false;
	}
	Debug("MS","Directory ");
	Debug(location);
	Debug(" created.\n");
	return true;
}

bool MassStorage::MakeDirectory(const char *directory)
{
	if (f_mkdir(directory) != FR_OK)
	{
		Debug("MS","Can't create directory ");
		Debug("MS",directory);
		Debug("\n");
		return false;
	}
	Debug("MS","Directory ");
	Debug(directory);
	Debug(" created.\n");
	return true;
}

// Rename a file or directory
bool MassStorage::Rename(const char *oldFilename, const char *newFilename)
{
	if (f_rename(oldFilename, newFilename) != FR_OK)
	{
		Debug("MS","Can't rename file or directory ");
		Debug("MS",oldFilename);
		Debug(" to ");
		Debug("MS",newFilename);
    Debug("MS","\n");
		return false;
	}
	Debug("MS ", oldFilename);
	Debug(" was renamed to ");
	Debug(newFilename);
	Debug("\n");
	return true;
}

// Check if the specified file exists
bool MassStorage::FileExists(const char *file) const
{
 	FILINFO fil;
 	fil.lfname = nullptr;
	if(f_stat(file, &fil) == FR_OK){
		Debug("MS","File ");
		Debug(file);
		Debug(" already exists.\n");
		return true;
	}
	Debug("MS","File ");
	Debug(file);
	Debug(" was not found.\n");
	return false;
}

// Check if the specified directory exists
bool MassStorage::PathExists(const char *path) const
{
 	DIR dir;
 	dir.lfn = nullptr;
	if(f_opendir(&dir, path) == FR_OK){
		Debug("MS","Path ");
		Debug(path);
		Debug(" already exists.\n");
		return true;
	}
	Debug("MS","Path ");
	Debug(path);
	Debug(" was not found.\n");
	return false;
}

bool MassStorage::PathExists(const char* directory, const char* subDirectory)
{
	const char* location = (directory != NULL)
							? SD.CombineName(directory, subDirectory)
								: subDirectory;
	return PathExists(location);
}

//------------------------------------------------------------------------------------------------

FileStore::FileStore(void)
{
}

void FileStore::Init()
{
	bufferPointer = 0;
	inUse = false;
	writing = false;
	lastBufferEntry = 0;
	openCount = 0;
}

// Open a local file (for example on an SD card).

bool FileStore::Open(const char* directory, const char* fileName, bool write)
{
	const char* location = (directory != NULL)
							? SD.CombineName(directory, fileName)
								: fileName;
	writing = write;
	lastBufferEntry = FILE_BUF_LEN - 1;
	bytesRead = 0;

//	FRESULT openReturn = f_open(&file, location, (writing) ? FA_CREATE_ALWAYS | FA_WRITE : FA_OPEN_EXISTING | FA_READ);
    FRESULT openReturn = f_open(&file, location, (writing) ?  FA_OPEN_ALWAYS | FA_WRITE : FA_OPEN_EXISTING | FA_READ);
	if (openReturn != FR_OK)
	{
		Debug("FS","Can't open ");
		Debug(location);
		Debug(" to ");
		Debug((writing) ? "write" : "read");
		Debug(", error code ");
		Debug(openReturn);
		Debug("\n");
		return false;
	}

	bufferPointer = (writing) ? 0 : FILE_BUF_LEN;
	inUse = true;
	openCount = 1;
	Debug("FS","File opened.\n");
	return true;
}

bool FileStore::CreateNew(const char* directory, const char* fileName)
{
	const char* location = (directory != NULL)
							? SD.CombineName(directory, fileName)
								: fileName;
	writing = true;
	lastBufferEntry = FILE_BUF_LEN - 1;
	bytesRead = 0;

	FRESULT openReturn = f_open(&file, location, (writing) ? FA_CREATE_ALWAYS | FA_WRITE : FA_OPEN_EXISTING | FA_READ);
	if (openReturn != FR_OK)
	{
		Debug("FS","Can't open ");
		Debug(location);
		Debug(" to ");
		Debug((writing) ? "write" : "read");
		Debug(", error code ");
		Debug(openReturn);
		Debug("\n");
		return false;
	}

	bufferPointer = 0;
	inUse = true;
	openCount = 1;
	Debug("FS","File created.\n");
	return true;
}


void FileStore::Duplicate()
{
	if (!inUse)
	{
		Debug("FS","Attempt to dup a non-open file.\n");
		return;
	}
	++openCount;
}

bool FileStore::Close()
{
	if (!inUse)
	{
		Debug("FS","Attempt to close a non-open file.\n");
		return false;
	}
	--openCount;
	if (openCount != 0)
	{
		Debug("FS","File closed.\n");
		return true;
	}
	bool ok = true;
	if (writing)
	{
		ok = Flush();
	}
	FRESULT fr = f_close(&file);
	inUse = false;
	writing = false;
	lastBufferEntry = 0;
	if(ok && fr == FR_OK){
		Debug("FS","File closed.\n");
	}
	return ok && fr == FR_OK;
}

unsigned long FileStore::Position() const
{
	return bytesRead;
}

bool FileStore::Seek(unsigned long pos)
{
	if (!inUse)
	{
		Debug("FS","Attempt to seek on a non-open file.\n");
		return false;
	}
	if (writing)
	{
		WriteBuffer();
	}
	FRESULT fr = f_lseek(&file, pos);
	if (fr == FR_OK)
	{
		bufferPointer = (writing) ? 0 : FILE_BUF_LEN;
		bytesRead = pos;
		return true;
	}
	return false;
}

bool FileStore::GoToEnd()
{
	return Seek(Length());
}

unsigned long FileStore::Length() const
{
	if (!inUse)
	{
		Debug("FS","Attempt to size non-open file.\n");
		return 0;
	}
	return file.fsize;
}

float FileStore::FractionRead() const
{
	unsigned long len = Length();
	if(len <= 0)
	{
		return 0.0;
	}

	return (float)bytesRead / (float)len;
}

int8_t FileStore::Status()
{
	if (!inUse)
		return nothing;

	if (lastBufferEntry == FILE_BUF_LEN)
		return byteAvailable;

	if (bufferPointer < lastBufferEntry)
		return byteAvailable;

	return nothing;
}

bool FileStore::ReadBuffer()
{
	FRESULT readStatus = f_read(&file, buf, FILE_BUF_LEN, &lastBufferEntry);	// Read a chunk of file
	if (readStatus)
	{
		Debug("FS","Error reading file.\n");
		return false;
	}
	bufferPointer = 0;
	return true;
}

// Single character read via the buffer
bool FileStore::Read(char& b)
{
	if (!inUse)
	{
		Debug("FS","Attempt to read from a non-open file.\n");
		return false;
	}

	if (bufferPointer >= FILE_BUF_LEN)
	{
		bool ok = ReadBuffer();
		if (!ok)
		{
			return false;
		}
	}

	if (bufferPointer >= lastBufferEntry)
	{
		b = 0;  // Good idea?
		return false;
	}

	b = (char) buf[bufferPointer];
	bufferPointer++;
	bytesRead++;

	return true;
}

// Block read, doesn't use the buffer
int FileStore::Read(char* extBuf, unsigned int nBytes)
{
	if (!inUse)
	{
		Debug("FS","Attempt to read from a non-open file.\n");
		return -1;
	}
	bufferPointer = FILE_BUF_LEN;	// invalidate the buffer
	UINT bytes_read;
	FRESULT readStatus = f_read(&file, extBuf, nBytes, &bytes_read);
	if (readStatus)
	{
		Debug("FS","Error reading file.\n");
		return -1;
	}
	bytesRead += bytes_read;
	return (int)bytes_read;
}

bool FileStore::WriteBuffer()
{
	if (bufferPointer != 0)
	{
		bool ok = InternalWriteBlock((const char*)buf, bufferPointer);
		if (!ok)
		{
			Debug("FS","Cannot write to file. Disc may be full.\n");
			return false;
		}
		bufferPointer = 0;
	}
	return true;
}

bool FileStore::Write(char b)
{
	if (!inUse)
	{
		Debug("FS","Attempt to write byte to a non-open file.\n");
		return false;
	}
	buf[bufferPointer] = b;
	bufferPointer++;
	if (bufferPointer >= FILE_BUF_LEN)
	{
		return WriteBuffer();
	}
	return true;
}

bool FileStore::Write(const char* b)
{
	if (!inUse)
	{
		Debug("FS","Attempt to write string to a non-open file.\n");
		return false;
	}
	int i = 0;
	while (b[i])
	{
		if (!Write(b[i++]))
		{
			return false;
		}
	}
	return true;
}

// Direct block write that bypasses the buffer. Used when uploading files.
bool FileStore::Write(const char *s, unsigned int len)
{
	if (!inUse)
	{
		Debug("FS","Attempt to write block to a non-open file.\n");
		return false;
	}
	if (!WriteBuffer())
	{
		return false;
	}
	return InternalWriteBlock(s, len);
}

bool FileStore::InternalWriteBlock(const char *s, unsigned int len)
{
 	unsigned int bytesWritten;
	uint32_t time = micros();
 	FRESULT writeStatus = f_write(&file, s, len, &bytesWritten);
	time = micros() - time;
	if (time > longestWriteTime)
	{
		longestWriteTime = time;
	}
 	if ((writeStatus != FR_OK) || (bytesWritten != len))
 	{
 		Debug("FS","Cannot write to file. Disc may be full.\n");
 		return false;
 	}
 	return true;
 }

bool FileStore::Flush()
{
	if (!inUse)
	{
		Debug("FS","Attempt to flush a non-open file.\n");
		return false;
	}
	if (!WriteBuffer())
	{
		return false;
	}
	return f_sync(&file) == FR_OK;
}

float FileStore::GetAndClearLongestWriteTime()
{
	float ret = (float)longestWriteTime/1000.0;
	longestWriteTime = 0;
	return ret;
}

uint32_t FileStore::longestWriteTime = 0;

MassStorage SD;

//***************************************************************************************************
