#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <dirent.h>

#include <3ds.h>

bool running = true;
bool menu_drawed = false;
bool error = false;
bool really = false;

char folders[100][100];

int mode = -1;
int selection = 0;
int dir_count = 0;
int sel_dir= 0;

PrintConsole topScreen, bottomScreen;

char* concat(char *s1, char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
    return result;
}

char* fillLine(char *s)
{
	int remain = 40;
	int i;
	remain = 40 - strlen(s);
	
	char tmp_s[100];
	char tmp_v[100];

	if(strlen(s) > 40){
		strncpy(tmp_v, s, 40);
		return tmp_v;
	}

	strcpy(tmp_s, s);

	for(i = 0; i < remain; i++)
	{
		strcat(tmp_s, " ");
	}

	return tmp_s;
}

u8 getSystemRegion()
{	
	u8 region = 0;
	CFGU_SecureInfoGetRegion(&region);
	return region;
	
}

int getAchievePath(u8 regioncode)
{	
	int achieve_data = -1;

	if(regioncode == 0){
		achieve_data = 0x00000209;
	}else if(regioncode == 1){
		achieve_data = 0x00000219;
	}else if(regioncode == 2){
		achieve_data = 0x00000229;
	}

	return achieve_data;
}

bool fsIsDirectory(char path[])
{
	DIR* dir = opendir(path);
	if(dir != NULL){
		closedir(dir);
		return true;
	}
	return false;
}

Result getFile(u8 ** buffer, u64 * size, char filepath[], int archieve) {
	Result ret = 0;
	Handle filehandle = 0;
	u32 tmpval = 0;

	u32 extdata_archive_lowpathdata[3] = { 1, archieve, 0 };
	FS_archive extdata_archive = (FS_archive ) { ARCH_EXTDATA,
					(FS_path ) { PATH_BINARY, 0xC,
									(u8*) extdata_archive_lowpathdata } };
	FSUSER_OpenArchive(NULL, &extdata_archive);

	ret = FSUSER_OpenFile(NULL, &filehandle, extdata_archive, FS_makePath(PATH_CHAR, filepath), FS_OPEN_READ, 0);

	ret = FSFILE_GetSize(filehandle, size);

	*buffer = malloc(*size);
	ret = FSFILE_Read(filehandle, &tmpval, 0, *buffer, *size);

	FSFILE_Close(filehandle);
	return ret;
}

Result putFile(u8 * buffer, u64 size, char filepath[], int archieve) {
	Result ret = 0;
	Handle filehandle = 0;
	u32 tmpval = 0;

	FS_path fspath = FS_makePath(PATH_CHAR, filepath);

	u32 extdata_archive_lowpathdata[3] = { 1, archieve, 0 };
	FS_archive extdata_archive = (FS_archive ) { ARCH_EXTDATA,
					(FS_path ) { PATH_BINARY, 0xC,
									(u8*) extdata_archive_lowpathdata } };
	FSUSER_OpenArchive(NULL, &extdata_archive);

	ret = FSUSER_DeleteFile(NULL, extdata_archive, fspath);

	ret = FSUSER_CreateFile(NULL, extdata_archive, fspath, size);
	if (ret != 0)
		return ret;

	ret = FSUSER_OpenFile(NULL, &filehandle, extdata_archive, fspath,
	FS_OPEN_WRITE, 0);
	if (ret != 0)
		return ret;

	ret = FSFILE_Write(filehandle, &tmpval, 0, buffer, size, FS_WRITE_FLUSH);
	FSFILE_Close(filehandle);

	return ret;
}

Result doBackup()
{	
	u8* u8file;
	u64 size;
	
	Result res = getFile(&u8file, &size, "/boss_bgm3", getAchievePath(getSystemRegion()));
	
	if(res != 0){
		
		
		printf("\x1b[97mNo backup found\x1b[0m\n");
		printf("\x1b[97mCreating backup, please wait...\x1b[0m\n\n");
		
		res = getFile(&u8file, &size, "/boss_bgm1", getAchievePath(getSystemRegion()));
		if(res != 0){
			printf("\x1b[97mCan't create backup... exiting\x1b[0m\n");
			printf("Initializion failed!");
			running = false;
			return res;
		}
		res = putFile(u8file, size, "/boss_bgm3", getAchievePath(getSystemRegion()));
		if(res == 0){
			printf("\x1b[97mboss_bgm backed up successfully\x1b[0m\n");	
		}else{
			printf("\x1b[97mFailed to write extdata... skipping\x1b[0m\n");
			return res;	
		}
		res = getFile(&u8file, &size, "/boss_xml1", getAchievePath(getSystemRegion()));
		res = putFile(u8file, size, "/boss_xml3", getAchievePath(getSystemRegion()));
		if(res == 0){
			printf("\x1b[97mboss_xml backed up successfully\x1b[0m\n\n");
			printf("\x1b[97mBackup created\x1b[0m");
		}
	

	}else{
		printf("\x1b[97mBackup found\x1b[0m\n");
	}

	gfxFlushBuffers();
	gfxSwapBuffers();

	return res;
}

Result restoreOrginalSong()
{
	u8* u8file;
	u64 size;

	consoleSelect(&bottomScreen);
	consoleClear();
	
	printf("\x1b[97mRestoring orginal song... Please wait\x1b[0m\n\n");
	Result res = getFile(&u8file, &size, "/boss_bgm3", getAchievePath(getSystemRegion()));
	
	if(res == 0){
		res = putFile(u8file, size, "/boss_bgm1", getAchievePath(getSystemRegion()));
		if(res != 0){
			selection = 0;
			mode = -1;

			gfxFlushBuffers();
			gfxSwapBuffers();
			printf("\x1b[97mFailed to write extdata... skipping\x1b[0m\n");
			return res;
		}else{
			printf("\x1b[97mboss_bgm successfully restored\x1b[0m\n");
		}

		Result res = getFile(&u8file, &size, "/boss_xml3", getAchievePath(getSystemRegion()));
		res = putFile(u8file, size, "/boss_xml1", getAchievePath(getSystemRegion()));
		if(res != 0){
			printf("\x1b[97mFailed to write extdata... skipping\x1b[0m\n");
			selection = 0;
			mode = -1;

			gfxFlushBuffers();
			gfxSwapBuffers();
			return res;
		}else{
			printf("\x1b[97mboss_xml successfully restored\x1b[0m\n\n");
			printf("\x1b[97mDone!");
		}

	}else{
		printf("\x1b[97mFile not found... exiting\x1b[0m");
	}
	
	selection = 0;
	mode = -1;

	gfxFlushBuffers();
	gfxSwapBuffers();

	return res;
	
}



void draw_menu()
{
	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();	
	
	consoleSelect(&topScreen);
	consoleClear();
	
	static const char* sel[] = {">"," "," "};	

	if(selection == 0){
		sel[0] = ">";
		sel[1] = " ";
		sel[2] = " ";
	}else if(selection == 1){
		sel[0] = " ";
		sel[1] = ">";
		sel[2] = " ";
	}else if(selection == 2){
		sel[0] = " ";
		sel[1] = " ";
		sel[2] = ">";
	}

	printf("\x1b[97m--------------------------------------------------\x1b[0m\n");
	printf("\x1b[97m|              eShop Music Changer               |\x1b[0m\n");
	printf("\x1b[97m--------------------------------------------------\x1b[0m\n");
	printf("\x1b[97m                   version 0.9b                   \x1b[0m\n\n");

	printf("\n\n\n\n");	
	
	if(mode == -1){
		printf("\x1b[97m          ------------------------------          \x1b[0m");
		printf("\x1b[97m          | Options                    |          \x1b[0m");
		printf("\x1b[97m          |----------------------------|          \x1b[0m");
		printf("\x1b[97m          |%s Select song               |          \x1b[0m", sel[0]);
		printf("\x1b[97m          |%s Restore orginal song      |          \x1b[0m", sel[1]);
		printf("\x1b[97m          |%s Exit                      |          \x1b[0m", sel[2]);
		printf("\x1b[97m          ------------------------------          \x1b[0m");
	}else if(mode == 0){
		printf("\x1b[97m          ------------------------------          \x1b[0m");
		printf("\x1b[97m          | Select a song              |          \x1b[0m");
		printf("\x1b[97m          |----------------------------|          \x1b[0m");
		printf("\x1b[97m          | Please select a song!      |          \x1b[0m");
		printf("\x1b[97m          |                            |          \x1b[0m");
		printf("\x1b[97m          | [A] Select       [B] Back  |          \x1b[0m");
		printf("\x1b[97m          ------------------------------          \x1b[0m");
		if(error){
			printf("\n");
		printf("\x1b[97m       Error: boss_bgm or boss_xml missing!       \x1b[0m");
		printf("\x1b[97m               Please select an other song        \x1b[0m");
		
		}
		
		if(really){
			printf("\n");
			printf("\x1b[97m             Confirm your selection             \x1b[0m\n");
			printf("\n\x1b[97m             [A] Confirm    [B] Back            \x1b[0m");
		
		}
	}else if(mode == 1){
		printf("\x1b[97m          ------------------------------          \x1b[0m");
		printf("\x1b[97m          | Restore orginal song       |          \x1b[0m");
		printf("\x1b[97m          |----------------------------|          \x1b[0m");
		printf("\x1b[97m          | Do you want to restore the |          \x1b[0m");
		printf("\x1b[97m          |       orginal song?        |          \x1b[0m");
		printf("\x1b[97m          | [A] Yes          [B] No    |          \x1b[0m");
		printf("\x1b[97m          ------------------------------          \x1b[0m");
	}
	gfxFlushBuffers();
	gfxSwapBuffers();
}

Result openDirectory()
{	
	Result res = 0;
	DIR* dir = opendir("./music");
	char temp[100][100];
	int temp_count = 0;
	
	if(dir != NULL){
		while(true){
			struct dirent* ent = readdir(dir);
			if(ent == NULL){
				break;
			}
			strcpy(temp[temp_count],ent->d_name);
			if(fsIsDirectory(concat("./music/", temp[temp_count]))){
				strcpy(folders[dir_count], temp[temp_count]);
				dir_count = dir_count + 1;
			}
			temp_count = temp_count + 1;	
		}	

	}else{
		res = -1;
		return res;
	}

	
	return res;
}

void selectFolder()
{
	
	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();	
	
	consoleSelect(&bottomScreen);
	consoleClear();
	//Select Folder
	int sel_count = 0;

	consoleSelect(&bottomScreen);
	printf("\n");
	for(sel_count = 0; sel_count < dir_count; sel_count++) {
		if(sel_count == sel_dir){
			printFolderList(1, fillLine(folders[sel_count]));
		}else{
			printFolderList(2, fillLine(folders[sel_count]));
		}
	}
	
	
}

void printFolderList(int mode, char folder[])
{
	if(mode == 1){
		printf(concat(concat("\x1b[47;30m",folder), "\x1b[0m\n"));
	}else if(mode == 2){
		printf(concat(concat("\x1b[40;37m",folder), "\x1b[0m\n"));
	}
	
}

Result checkForFiles()
{
	Result res;
	res = 0;
	
	FILE *file = fopen(concat(concat("./music/",folders[sel_dir]),"/boss_bgm.aac"),"rb");
	
	if (file == NULL){
		res = -1;
		return res;
	}
	fclose(file);
	file = fopen(concat(concat("./music/",folders[sel_dir]),"/boss_xml.xml"),"rb");
	if (file == NULL){
		res = -1;
		return res;
	}
	
	return res;
}

void confirmSelection()
{
	Result res = checkForFiles();
	if(res == 0){
		really = true;
		error = false;
	}else{
		error = true;
		really = false;
	}
}

Result replaceMusic()
{
	Result res = 0;
	FILE *bgm = fopen(concat(concat("./music/",folders[sel_dir]),"/boss_bgm.aac"),"rb");
	FILE *xml = fopen(concat(concat("./music/",folders[sel_dir]),"/boss_xml.xml"),"rb");

	u8* buffer_bgm;
	u8* buffer_xml;

	fseek(bgm,0,SEEK_END);
	fseek(xml,0,SEEK_END);

	off_t bgm_size = ftell(bgm);
	off_t xml_size = ftell(xml);

	fseek(bgm,0,SEEK_SET);
	fseek(xml,0,SEEK_SET);

	buffer_bgm = malloc(bgm_size);
	if(!buffer_bgm){
		res = -1;
		return res;
	}

	buffer_xml = malloc(bgm_size);
	if(!buffer_xml){
		res = -1;
		return res;
	}

	off_t bytesRead_bgm = fread(buffer_bgm,1,bgm_size,bgm);
	off_t bytesRead_xml = fread(buffer_xml,1,xml_size,xml);
	
	if(bgm_size != bytesRead_bgm){
		res = -1;
		return res;
	}
	if(xml_size != bytesRead_xml){
		res = -1;
		return res;
	}
	
	printf("\x1b[97mReplacing song... Please wait\x1b[0m\n\n");
	printf("Replacing boss_bgm...\n");
	
	res = putFile(buffer_bgm, bgm_size, "/boss_bgm1", getAchievePath(getSystemRegion()));
	if(res == 0){
		printf("Replacing boss_xml...\n\n");
		res = putFile(buffer_xml, xml_size, "/boss_xml1", getAchievePath(getSystemRegion()));
		if(res !=0){
			printf("Failed!");
			return res;	
		}
	}else{
		printf("Failed!");
		return res;
	}

	printf("Done!\n");

	fclose(bgm);
	fclose(xml);

	mode = -1;
	really = 0;
	error = 0;
	selection = 0;
	sel_dir = 0;

	return res;
}

void executeCommand(u8 key)
{
	if(mode == -1){
		if(selection == 0 && key == 1){
			mode = 0;
			selectFolder();
		}else if(selection == 1 && key == 1){
			mode = 1;
		}else if(selection == 2 && key == 1){
			running = false;
		}
	}else if(mode == 0){
		if(key == 1){
			if(really == false){
				confirmSelection();
			}else{
				consoleSelect(&bottomScreen);
				consoleClear();
				replaceMusic();
			}
		}else{
			if(really == false){
				mode = -1;
				really = 0;
				error = 0;
				selection = 0;
				sel_dir = 0;
				consoleSelect(&bottomScreen);
				consoleClear();
			}else{
				really = false;
			}
		}
	}else if(mode == 1){
		if(key == 1){
			restoreOrginalSong();
		}else{
			mode = -1;
			selection = 0;
		}
	}
	
}

int main()
{	
	//Variables
		
	
	// Initialize services
	gfxInitDefault();
	initCfgu();
	fsInit();

	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);
	
	consoleSelect(&topScreen);
	printf("\x1b[97meShop Music Changer\x1b[0m\n\n");
	printf("\x1b[97mInitialize...\x1b[0m\n\n");

	doBackup();
	openDirectory();
	svcSleepThread(2000000000LL);

	gfxFlushBuffers();
	gfxSwapBuffers();

	draw_menu();
	
	// Main loop
	while (aptMainLoop() && running)
	{
		gspWaitForVBlank();
		hidScanInput();

		// Your code goes here

		u32 kDown = hidKeysDown();

		if (kDown & KEY_DOWN){
			if(selection < 2){
				selection = selection +1;
			}
			if(mode == -1){
				draw_menu();	
			}else if(mode == 0 && really == false){
				if(sel_dir < dir_count-1){
					sel_dir++;
					selectFolder();
				}
			}
		}else if(kDown & KEY_UP){
			if(selection > 0){
				selection = selection -1;
			}
			if(mode == -1){
				draw_menu();	
			}else if(mode == 0 && really == false){
				if(sel_dir > 0){
					sel_dir--;
					selectFolder();
				}
			}
		}else if(kDown & KEY_A){
			executeCommand(1);
			draw_menu();
		}else if(kDown & KEY_B){
			executeCommand(2);
			draw_menu();
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	return 0;
}
