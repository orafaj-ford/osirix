//
//  N2ManagedDatabase.mm
//  OsiriX
//
//  Created by Alessandro Volz on 17.01.11.
//  Copyright 2011 OsiriX Team. All rights reserved.
//

#import "N2ManagedDatabase.h"


@interface N2ManagedDatabase ()

@property(readwrite,retain) NSString* sqlFilePath;
@property(readwrite,retain) NSManagedObjectContext* managedObjectContext;

@end


@implementation N2ManagedDatabase

@synthesize sqlFilePath = _sqlFilePath, managedObjectContext = _managedObjectContext;

-(NSManagedObjectModel*)managedObjectModel {
	[NSException raise:NSGenericException format:@"[%@ managedObjectModel] must be defined", self.className];
	return NULL;
}

-(NSMutableDictionary*)persistentStoreCoordinatorsDictionary {
	static NSMutableDictionary* dict = NULL;
	if (!dict)
		dict = [[NSMutableDictionary alloc] initWithCapacity:4];
	return dict;
}

-(BOOL)migratePersistentStoresAutomatically {
	return YES;
}

-(NSManagedObjectContext*)contextAtPath:(NSString*)sqlFilePath {
	sqlFilePath = sqlFilePath.stringByExpandingTildeInPath;
	
    NSManagedObjectContext* moc = [[NSManagedObjectContext alloc] init];
	moc.undoManager = nil;
	
	@synchronized (self) {
		moc.persistentStoreCoordinator = self.managedObjectContext.persistentStoreCoordinator;
		if (!moc.persistentStoreCoordinator) {
			moc.persistentStoreCoordinator = [self.persistentStoreCoordinatorsDictionary objectForKey:sqlFilePath];
			
			BOOL isNewFile = ![NSFileManager.defaultManager fileExistsAtPath:sqlFilePath];
			if (isNewFile)
				moc.persistentStoreCoordinator = nil;
			
			if (!moc.persistentStoreCoordinator) {
				moc.persistentStoreCoordinator = [[[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:self.managedObjectModel] autorelease];
				[self.persistentStoreCoordinatorsDictionary setObject:moc.persistentStoreCoordinator forKey:sqlFilePath];
		
				NSURL* url = [NSURL fileURLWithPath:sqlFilePath];
				NSDictionary* options = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:[self migratePersistentStoresAutomatically]], NSMigratePersistentStoresAutomaticallyOption, NULL]; // [NSNumber numberWithBool:YES], NSInferMappingModelAutomaticallyOption, NULL];
				NSError* err = NULL;
				if (![moc.persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:NULL URL:url options:options error:&err]) {
					NSLog(@"Error: [N2ManagedDatabase contextAtPath:] %@", err);
					NSRunCriticalAlertPanel(NSLocalizedString(@"Database Error", NULL), err.localizedDescription, NSLocalizedString(@"OK", NULL), NULL, NULL);
					
					// error = [NSError osirixErrorWithCode:0 underlyingError:error localizedDescriptionFormat:NSLocalizedString(@"Store Configuration Failure: %@", NULL), error.localizedDescription? error.localizedDescription : NSLocalizedString(@"Unknown Error", NULL)];
					
					// delete the old file...
					[NSFileManager.defaultManager removeItemAtPath:sqlFilePath error:NULL];
					// [NSFileManager.defaultManager removeItemAtPath: [defaultPortalUsersDatabasePath.stringByExpandingTildeInPath.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"WebUsers.vers"] error:NULL];
					
					[moc.persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:NULL URL:url options:options error:NULL];
				}
			}
			
			// this line is very important, if there is no sql file
			[moc save:NULL];
			
			if (isNewFile)
				NSLog(@"New database file created at %@", sqlFilePath);
		}
	}

    return [moc autorelease];
}

-(void)lock {
	[self.managedObjectContext lock];
}

-(BOOL)tryLock {
	return [self.managedObjectContext tryLock];
}

-(void)unlock {
	[self.managedObjectContext unlock];
}

//-(void)writeLock {
//	[writeLock lock];
//}
//
//-(BOOL)tryWriteLock {
//	return [writeLock tryLock];
//}
//
//-(void)writeUnlock {
//	[writeLock unlock];
//}

-(id)initWithPath:(NSString*)p context:(NSManagedObjectContext*)c {
	self = [super init];
//	writeLock = [[NSRecursiveLock alloc] init];
	
	self.sqlFilePath = p;
	
	if (!c)
		c = [self contextAtPath:p];
	self.managedObjectContext = c;
	
	return self;
}

-(id)initWithPath:(NSString*)p {
	return [self initWithPath:p context:nil];
}

-(void)dealloc {
//	[self.managedObjectContext reset];
	self.managedObjectContext = nil;
	self.sqlFilePath = nil;
//	[writeLock release];
	[super dealloc];
}

-(NSManagedObjectContext*)independentContext:(BOOL)independent {
	return independent? [self contextAtPath:self.sqlFilePath] : self.managedObjectContext;
}

-(NSManagedObjectContext*)independentContext {
	return [self independentContext:YES];
}

-(NSEntityDescription*)entityForName:(NSString*)name {
	return [NSEntityDescription entityForName:name inManagedObjectContext:self.managedObjectContext];
}

-(NSManagedObject*)objectWithID:(NSString*)theId {
	return [self.managedObjectContext objectWithID:[self.managedObjectContext.persistentStoreCoordinator managedObjectIDForURIRepresentation:[NSURL URLWithString:theId]]];
}

-(NSArray*)objectsForEntity:(NSEntityDescription*)e predicate:(NSPredicate*)p {
	return [self objectsForEntity:e predicate:p error:NULL];
}

-(NSArray*)objectsForEntity:(NSEntityDescription*)e predicate:(NSPredicate*)p error:(NSError**)err {
	NSFetchRequest* req = [[[NSFetchRequest alloc] init] autorelease];
	req.entity = e;
	req.predicate = p;
	return [self.managedObjectContext executeFetchRequest:req error:err];
}

-(id)newObjectForEntity:(NSEntityDescription*)entity {
	return [[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:self.managedObjectContext];
}

-(void)save:(NSError**)err {
	NSError* perr = NULL;
	if (!err) err = &perr;
	[self lock];
	@try {
		[self.managedObjectContext save:err];
	} @catch(NSException* e) {
		if (!*err)
			*err = [NSError errorWithDomain:@"Exception" code:-1 userInfo:[NSDictionary dictionaryWithObject:e forKey:@"Exception"]];
	} @finally {
		[self unlock];
	}
}


@end