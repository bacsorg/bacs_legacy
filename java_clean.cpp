#include <stdio.h>
#include <stdlib.h> 

int main( int argc, char ** argv )
{
	if ( argc != 2 )
	{
		fprintf( stderr, "Error: Argc(%d) != 2, run 'java_clean JAVA_FILE'\n", argc ); 
		return 1;
	}
	
	char buf[1024];
	sprintf( buf, "rm %s.dir/*", argv[1] );
	system( buf );
	sprintf( buf, "rmdir %s.dir", argv[1] );
	system( buf );
		
	return 0;
}
