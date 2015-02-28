#include "fat.h"

void Fill_BPB() //llena la estructura BPB
{
	read(file,&boot,sizeof(struct BPB));
	return;
}

int FindNextCluster(int cluster) //devuelvo el proximo cluster, al llamarlo ya estoy parado en la FAT
{
	int clusterRes;
	lseek(file,cluster*4,SEEK_CUR); //me paro en la FAT en donde esta el prox cluster
	read(file,&clusterRes,sizeof(int)); //leo la pos del proximo cluster
	if(clusterRes==FreeClust || clusterRes==WrongClust || clusterRes==RervClust || clusterRes==ResvClust1 || clusterRes==ResvClust2)
	{
		perror("bateo con el cluster");
		return -1;
	}
	return clusterRes & FourReserved;
}

void FillName(char* name,DirEntry dir) //lleno el nombre de las entradas largas
{
    //esto se hace porque si es la ultima entrada larga correspondiente del dir se activa una flag que amuenta LDIR_Ord en 64
    if(dir.LongDirEntry.LDIR_Ord>=64) dir.LongDirEntry.LDIR_Ord-=64;

	int i=0;
	int k=0; //por donde estoy llenando en name
	for(i;i<10;i+=2,k++)
	{
		    if(dir.LongDirEntry.LDIR_Name1[i]==255) return; //si llegue al final
			name[(dir.LongDirEntry.LDIR_Ord-1)*13+k]=dir.LongDirEntry.LDIR_Name1[i];
    }
    i=0;
    for(i;i<12;i+=2,k++) //porque cojo uno si y uno no
	{
		    if(dir.LongDirEntry.LDIR_Name2[i]==255) return;
			name[(dir.LongDirEntry.LDIR_Ord-1)*13+(k)]=dir.LongDirEntry.LDIR_Name2[i];
    }
    i=0;
    for(i;i<4;i+=2,k++)
	{
		    if(dir.LongDirEntry.LDIR_Name3[i]==255) return;
			name[(dir.LongDirEntry.LDIR_Ord-1)*13+(k)]=dir.LongDirEntry.LDIR_Name3[i];
    }
}

unsigned int ToInt(unsigned short hi,unsigned short lo) //Convierte 2 shorts en un int
{
	int hi1=hi;
	int hi2=lo;
	hi1=(hi1<<0x10)|hi2;
	return hi1;
}

int SameChksum(char chksum,int primeraLarga) //Devuelve 1 si tiene iguales chksum y -1 eoc
{
	if(primeraLarga==1) //si es la primera entrada larga
	{
		myChksum=chksum; //actualizo el chksum
		primVez=2;
		return 1;
	}
	else
	{
		if(chksum!=myChksum) return -1;
		return 1;
	}
}

int CountOfLink(int clusterAux) //le paso el cluster donde empiezan todos los ficheros hijos,devuelve la cant de enlaces
{

	int myIndex=((clusterAux-2) * boot.BPB_SecPerClus)*boot.BPB_BytsPerSec + FirstDataSector; //calculo la pos del primer sector del cluster
	int pos=lseek(file,myIndex,SEEK_SET); //me paro en el primer sector de datos del cluster
	if(pos!=myIndex)
		perror("Bateo con la lectura");
	int countShort=0; //cantidad de entradas cortas
	int countEntry=0; //cantidad de entradas
    MyEntry actEntry; //entrada que voy a ir llenando

    while(1)
    {
		read(file,&(actEntry.data),sizeof(union DirEntry)); //leo la entrada de directorio
		countEntry++;
		if(actEntry.data.ShortDirEntry.DIR_Name[0]==0xE5)
		{
			continue; //el cluster esta vacio
		}
		if(actEntry.data.ShortDirEntry.DIR_Name[0]==0x00)
		{
			return countShort; //de aqui pa abajo no hay mas na
		}
		if(actEntry.data.LongDirEntry.LDIR_Attr!=ATTR_LONG_NAME) //pregunto si es una entrada corta
			countShort++;
		if(countEntry==(boot.BPB_BytsPerSec*boot.BPB_SecPerClus)/32) //si llegue al final del cluster
		{
				//voy a cargar el proximo bloque
				long FatPos=boot.BPB_RsvdSecCnt*boot.BPB_BytsPerSec; //calculo la pos en disco de la FAT
				pos =  lseek(file,FatPos,SEEK_SET); //me paro en la FAT
				if(pos!=FatPos)
				 perror("Bateo");
				clusterAux = FindNextCluster(clusterAux); //busco el proximo cluster
				//preguntar si el cluster tiene problemas
				if(clusterAux==-1) return countShort;
				int index = FirstSectorCluster(clusterAux); //calculo la pos del primer sector del cluster
				lseek(file,index,SEEK_SET); //me paro en el primer sector del cluster
				countEntry=0;
				continue;
		}
	}
	return countShort;
}

int FirstSectorCluster(int cluster)
{
	return ((cluster-2) * boot.BPB_SecPerClus)*boot.BPB_BytsPerSec + FirstDataSector; //en bytes
}

MyEntry* GiveMeDirEntry(char *path)
{

	struct MyEntry * r=(struct MyEntry*) malloc(sizeof(MyEntry));
	long begin = FirstDataSector; //calculo la dir del primer sector de datos
	long pos=lseek(file,begin,SEEK_SET); //me paro en el primer sector de datos
	struct MyEntry result=*r;
	if(pos!=begin)
		perror("Bateo");

    //para que no me de segmentation fault
    char * Copy =malloc(255);
    strcpy(Copy,path);

	char* token=strtok(Copy,"/"); //guardo los tokens

	if(token==NULL) //pregunto si es el root
	{
		//tengo que llenarlo a mano
		strcpy(result.data.ShortDirEntry.DIR_Name,"DIR-ROOT");
		result.data.ShortDirEntry.DIR_Attr = ATTR_DIRECTORY;
	    result.data.ShortDirEntry.DIR_NTRes = 0;
	   	result.data.ShortDirEntry.DIR_CrtTime = 0;
	   	result.data.ShortDirEntry.DIR_CrtTimeTenth = 0;
	   	result.data.ShortDirEntry.DIR_CrtDate = 0;
	   	result.data.ShortDirEntry.DIR_FstClusHI = 0;
    	result.data.ShortDirEntry.DIR_FstClusLO = boot.BPB_RootClus;
    	strcpy(result.name,"DIR-ROOT");
		free(Copy); //libero copy
	    return &result;
	}
	currentCluster=2; //probar bpbrootclus
	//int p=2; //primer cluster de datos
	int count=0; //cantidad de entradas de directorios
	char nombre[255];
	memset(nombre,0,255);

	 //nombre del directorio
	while(token!=NULL) //mientras pueda coger tokens
	{
		read(file,&(result.data),sizeof(union DirEntry)); //lleno la entrada de directorio
		count++;

		if(result.data.ShortDirEntry.DIR_Name[0]==0xE5)
		{
			continue; //el dirEntry esta vacio
		}
		if(result.data.ShortDirEntry.DIR_Name[0]==0x00)
		{
			return NULL; //de aqui pa abajo no hay mas na
		}
		while(result.data.LongDirEntry.LDIR_Attr == ATTR_LONG_NAME) //mientras que sea una entrada larga
		{
			if(SameChksum(result.data.LongDirEntry.LDIR_Chksum,primVez)!=1) //verifico si tienen el mismo chksum
				perror("Bateo con el chksum");

			FillName(nombre,result.data); //guardo los pedazos de nombre de la entrada larga

			if(count==(boot.BPB_BytsPerSec*boot.BPB_SecPerClus)/32) //si llegue al final del cluster
			{
				//voy a cargar el proximo bloque
				long FatPos=boot.BPB_RsvdSecCnt*boot.BPB_BytsPerSec; //calculo la pos en disco de la FAT
				pos =  lseek(file,FatPos,SEEK_SET); //me paro en la FAT
				if(pos!=FatPos)
				 perror("Bateo");
				currentCluster = FindNextCluster(currentCluster); //busco el proximo cluster
				//preguntar si el cluster tiene problemas
				if(currentCluster==-1) return NULL;
				int index=FirstSectorCluster(currentCluster); //calculo la pos del prox cluster
				lseek(file,index,SEEK_SET); //me paro en el proximo cluster
				count=0;

				continue;
			}
			memset(&(result.data),0,sizeof(union DirEntry)); //pongo en cero pa na
			read(file,&(result.data),sizeof(union DirEntry)); //lleno la entrada de directorio
			count++; //procese una entrada larga mas
			//primVez=2;
		}

		primVez=1; //lo proximo puede ser la primera larga de un set
		//es una entrada corta
		if(strcmp(nombre,token)!=0) //comparo si el nombre que tengo es igual al del token
		{
			//seguir buscando
			memset(nombre,0,255);
			continue;
		}
		else
		{
			//guardo la parte alta y la baja
			currentCluster = ToInt(result.data.ShortDirEntry.DIR_FstClusHI,result.data.ShortDirEntry.DIR_FstClusLO); //me da el num de cluster
			int index = FirstSectorCluster(currentCluster); //cojo la pos del primer sector del cluster
			lseek(file,index,SEEK_SET); //me paro en el primer sector del cluster
			//cojo el proximo token
			memset(nombre,0,255);
			token=strtok(NULL,"/");
		}
	}
	return &result;
}

int ComparePto(char * nombre1, char * nombre2,int longitud)
{
	int i=0;
	for (i; i < longitud; i++)
	{
		if(nombre1[i]!=nombre2[i])
		 return -1;
	}
	return 1;
}

time_t GetTime(short time,short date)
{
	//sacado de la documentacion de fat32
	struct tm * t;
	t= malloc(sizeof(struct tm));
	memset(t,0,sizeof(struct tm));

	t->tm_sec=((((uint8_t *) &(time))[0] & 0x1f) * 2);
	t->tm_min=((((((uint8_t *) &(time))[1]&0x7) << 3) + (((uint8_t *) &(time))[0] >> 5)));
	t->tm_hour=(((uint8_t *) &(time))[1] >> 3);
	t->tm_mday=(((uint8_t *) &(date))[0] & 0x1f);
	t->tm_mon=((((((uint8_t *) &(date))[1]&0x1) << 3) + (((uint8_t *) &(date))[0] >> 5)))-1;
	t->tm_year=(( ((uint8_t *) &(date))[1] >> 1) + 80);
	t->tm_gmtoff = 0;
	t->tm_isdst = -1;
 	return mktime(t);
}

int FillStatStruct(MyEntry * entry,struct stat *stbuf)
{
	memset(stbuf,0,sizeof(struct stat)); //lleno stat con ceros para que no malinterprete nada
	short n=((entry->data.ShortDirEntry.DIR_Attr) & (ATTR_DIRECTORY|ATTR_VOLUME_ID)); //es para saber si es un directorio o un volumen
	if(n==ATTR_DIRECTORY)
	{
		//es un directiorio
		int clus=ToInt(entry->data.ShortDirEntry.DIR_FstClusHI,entry->data.ShortDirEntry.DIR_FstClusLO); //busco la pos del cluster donde esta la inf de los ficheros hijos
		stbuf->st_nlink=CountOfLink(clus); //la cantidad de enlaces fuertes
		stbuf->st_mode=S_IFDIR; //el modo=>es un directorio
	}
	else
	{
		if(n==0) //pregunto si es un fichero=> no directorio y no volumen
		{
			stbuf->st_nlink=1; //tiene un solo enlace duro
			stbuf->st_mode=S_IFREG;
		}
	}

	//para llenar los permisos
	if((entry->data.ShortDirEntry.DIR_Attr & ATTR_READ_ONLY)==ATTR_READ_ONLY)
		stbuf->st_mode|= 0444; //solo lectura
	else
		stbuf->st_mode|= 0755; //lectura y escritura

	//tamano del fichero
	stbuf->st_size = entry->data.ShortDirEntry.DIR_FileSize;
	stbuf->st_atime = GetTime(0,entry->data.ShortDirEntry.DIR_LstAccDate);
	stbuf->st_mtime = GetTime(entry->data.ShortDirEntry.DIR_WrtTime, entry->data.ShortDirEntry.DIR_WrtDate);
	stbuf->st_ctime = GetTime(entry->data.ShortDirEntry.DIR_CrtTime, entry->data.ShortDirEntry.DIR_CrtDate);
	stbuf->st_ino = ToInt(entry->data.ShortDirEntry.DIR_FstClusHI,entry->data.ShortDirEntry.DIR_FstClusLO);

	return 0;
}

static int fat_getattr(const char *path, struct stat *stbuf)
{
	MyEntry * actEntry =GiveMeDirEntry(path); //busco la entrada de directorio
	if(actEntry==NULL) return -ENOENT; //error en la entrada

    return FillStatStruct(actEntry,stbuf); //lleno la estructura stat
}

static int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	MyEntry * entry = GiveMeDirEntry(path);
	if(entry==NULL) return -ENOENT; //error en la entrada

	char nombre[255];
	memset(nombre,0,255); //lo lleno de ceros
	int count=0;

	int cluster=ToInt(entry->data.ShortDirEntry.DIR_FstClusHI,entry->data.ShortDirEntry.DIR_FstClusLO);
	int index=FirstSectorCluster(cluster);
	int pos = lseek(file,index,SEEK_SET); //me pongo en el primer sector del cluster
	if(pos!=index)
		perror("Bateo con la lectura");

	while(1)
	{
		read(file,&(entry->data),sizeof(union DirEntry)); //leo la entrada de directorio
		count++;

		if(entry->data.ShortDirEntry.DIR_Name[0]==0xE5)
		{
			continue; //el cluster esta vacio
		}
		if(entry->data.ShortDirEntry.DIR_Name[0]==0x00)
		{
			return 0; //de aqui pa abajo no hay mas na
		}

		while(entry->data.LongDirEntry.LDIR_Attr == ATTR_LONG_NAME) //mientras que sea una entrada larga
		{
			if(SameChksum(entry->data.LongDirEntry.LDIR_Chksum,primVez)!=1) //verifico si tienen el mismo chksum
				perror("Bateo con el chksum");

			FillName(nombre,entry->data); //guardo los pedazos de nombre de la entrada larga

			if(count==(boot.BPB_BytsPerSec*boot.BPB_SecPerClus)/32) //si llegue al final del cluster
			{
				//voy a cargar el proximo bloque
				long FatPos=boot.BPB_RsvdSecCnt*boot.BPB_BytsPerSec; //calculo la pos en disco de la FAT
				int pos =  lseek(file,FatPos,SEEK_SET); //me paro en la FAT
				if(pos!=FatPos)
				 perror("Bateo");
				currentCluster = FindNextCluster(currentCluster); //busco el proximo cluster
				//preguntar si el cluster tiene problemas
				if(currentCluster==-1) return 0;
				int index = FirstSectorCluster(currentCluster); //calculo la pos del primer sector del primer sector del cluster
				lseek(file,index,SEEK_SET); //me paro en el primer sector del cluster
				read(file,&(entry->data),sizeof(union DirEntry)); //lleno la entrada de directorio
				count=1; //va a ser la primera entrada
				continue;
			}
			read(file,&(entry->data),sizeof(union DirEntry)); //lleno la entrada de directorio
			count++; //procese una entrada larga mas
		}
		primVez=1; //lo proximo puede ser la primera larga
		if(ComparePto(entry->data.ShortDirEntry.DIR_Name,".          ",11)==1 || ComparePto(entry->data.ShortDirEntry.DIR_Name,"..         ",11)==1)
			filler(buf,entry->data.ShortDirEntry.DIR_Name,NULL,0);
		else
		{
			//es una entrada corta voy a copiar a filler
			filler(buf,nombre,NULL,0);
			memset(nombre,0,255);
		}
	}
    return 0;
}

int NumClus(int countClus,int primerCluster)
{
	long FatPos;
	int pos;
	int clusAct=primerCluster; //el actual es donde empieza la informacion
	FatPos=boot.BPB_RsvdSecCnt*boot.BPB_BytsPerSec; //calculo la pos en el sistema de la FAT

	while(countClus>0)
	{
		pos=lseek(file,FatPos,SEEK_SET); //me paro en la FAT
		if(pos!=FatPos)
		{
			perror("Bateo con la lectura");
			return clusAct;
		}
		clusAct = FindNextCluster(clusAct); //busco el proximo cluster
		countClus--; //me falta un cluster menos
	}
	return clusAct;
}

static int fat_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	MyEntry * res = GiveMeDirEntry(path); //busco la entrada
	if(res ==NULL) return -ENOENT;

	if(offset>res->data.ShortDirEntry.DIR_FileSize)
		return 0;

	int index=ToInt(res->data.ShortDirEntry.DIR_FstClusHI,res->data.ShortDirEntry.DIR_FstClusLO); //busco el primer cluster donde empieza la informacion de los hijos de la entrada

	int countCluster=offset/(boot.BPB_BytsPerSec*boot.BPB_SecPerClus); //me da el cluster n que tengo que empezar a read

	//me fui pa la fat
	int cluster = NumClus(countCluster,index); //me da el cluster donde tengo que empezar a leer

	int rem=offset%(boot.BPB_BytsPerSec*boot.BPB_SecPerClus); //me da la pos dentro del cluster donde tengo que leer

	int sizeFile=res->data.ShortDirEntry.DIR_FileSize-((countCluster*boot.BPB_BytsPerSec*boot.BPB_SecPerClus) + rem); //tamano del fichero que leer
	int tamClus=(boot.BPB_BytsPerSec*boot.BPB_SecPerClus)-(rem); //tamano del cluster por leer
	int countRead=0; //la cantidad de bytes que estoy leyendo
	int fatPos=boot.BPB_RsvdSecCnt*boot.BPB_BytsPerSec; //calculo la pos en disco de la FAT
	int pos; //para saber si hubo errores
	char *bf=buf;

	while(1)
	{
		int min=Min(size,sizeFile,tamClus); //tengo el min
		lseek(file,FirstSectorCluster(cluster) + rem,SEEK_SET); //me pongo en cluster pa read
		read(file,bf,min); //leo la cantidad minima
		bf+=min; //muevo el puntero
		size-=min; //actualizo size
		sizeFile-=min; //actualizo sizeFile
		countRead+=min; //lei una cantidad=min mas que la que tenia
		rem=0; //a partir de ahora empiezo a leer al principio del cluster
		if(size<=0 || sizeFile<=0) //si acabe de leer
			return countRead;

		//sino voy pa otro cluster
		tamClus=boot.BPB_BytsPerSec*boot.BPB_SecPerClus; //porque estoy parado al principio del cluster
		pos=lseek(file,fatPos,SEEK_SET); //me paro en la FAT
		if(pos!=fatPos)
		{
			perror("bateo con la lectura");
			return countRead;
		}
		cluster = FindNextCluster(cluster); //busco el proximo cluster

		if(cluster==-1) return countRead; //pregunto si esta bien el cluster
	}
}

int Min(int sizeOff,int sizeFile,int sizeClus)
{
	int min;
	if(sizeOff<sizeFile) min=sizeOff;
	else min=sizeFile;
	if(min>sizeClus) min=sizeClus;
	return min;
}

static int fat_open(const char *path, struct fuse_file_info *fi)
{
	if(strlen(path)> 255)
		return -ENAMETOOLONG;
	MyEntry *res = GiveMeDirEntry(path);
	if(res==NULL)
	{
	  return 0;
	}

    return 0;
}

static struct fuse_operations fat_oper = {
    .getattr	= fat_getattr,
    .readdir	= fat_readdir,
    .open	= fat_open,
    .read	= fat_read,
};

int main(int argc, char *argv[])
{
	file=open(argv[2],O_RDONLY); //abro el fichero
	Fill_BPB(); //lleno la estructura BPB
	FirstDataSector=(boot.BPB_RsvdSecCnt+(boot.BPB_NumFATs*boot.BPB_FATSz32))*boot.BPB_BytsPerSec; //en bytes
	myChksum=0;
	primVez=1;
	argc--;
	//argv[2]="-d";
    return fuse_main(argc, argv, &fat_oper);
}

