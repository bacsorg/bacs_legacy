#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h> 

bool file_exists( const char *fn )
{
  struct stat stFileInfo;
  bool blnReturn;
  int intStat;

  // Attempt to get the file attributes
  intStat = stat( fn, &stFileInfo );
  if(intStat == 0) {
    // We were able to get the file attributes
    // so the file obviously exists.
    blnReturn = !S_ISDIR( stFileInfo.st_mode );
  } else {
    // We were not able to get the file attributes.
    // This may mean that we don't have permission to
    // access the folder which contains this file. If you
    // need to do that level of checking, lookup the
    // return values of stat which will give you
    // more details on why stat failed.
    blnReturn = false;
  }
  
  return (blnReturn);
}
int main( int argc, char ** argv )
{
	if ( argc != 3 )
	{
		fprintf( stderr, "Error: Argc(%d) != 2, run 'java_compile JAVA_FILE NEW_NAME'\n", argc ); 
		return 1;
	}
	if ( !file_exists( argv[1] ) )
	{
		fprintf( stderr, "Error: File not found, JAVA_FILE = %s'\n", argv[1] ); 
		return 1;
	}
	
	char buf[1024];
	sprintf( buf, "mkdir %s.dir", argv[1] );
	system( buf );
	sprintf( buf, "ln %s %s.dir/%s", argv[1], argv[1], argv[2] );
	system( buf );
	sprintf( buf, "javac -g:none -classpath %s.dir %s.dir/%s", argv[1], argv[1], argv[2] );
	system( buf );
	sprintf( buf, "rm %s.dir/%s", argv[1], argv[2] );
	system( buf );
		
	return 0;
}
