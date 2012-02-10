/*=========================================================================
 Program:   OsiriX
 
 Copyright (c) OsiriX Team
 All rights reserved.
 Distributed under GNU - LGPL
 
 See http://www.osirix-viewer.com/copyright.html for details.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.
 =========================================================================*/

#import "N2ManagedDatabase.h"
#import "NSMutableDictionary+N2.h"
#import "N2Debug.h"


@interface N2ManagedDatabase ()

@property(readwrite,retain) NSString* sqlFilePath;
//@property(readwrite,retain) NSManagedObjectContext* managedObjectContext;

@end


@interface N2ManagedObjectContext : NSManagedObjectContext {
	N2ManagedDatabase* _database;
}

@property(retain) N2ManagedDatabase* database;

@end

@implementation N2ManagedObjectContext

@synthesize database = _database;

-(void)dealloc {
//	NSLog(@"---------- DEL %@", self);
	self.database = nil;
	[super dealloc];
	[NSNotificationCenter.defaultCenter removeObserver:self]; // Apple bug? It seems the managedObjectContext gets notified by the persistentStore, and the notifications are still sent after the context's dealloc..
}

@end


@implementation N2ManagedDatabase

@synthesize sqlFilePath = _sqlFilePath, managedObjectContext = _managedObjectContext;

-(NSManagedObjectContext*)managedObjectContext {
	return _managedObjectContext;
}

-(void)setManagedObjectContext:(NSManagedObjectContext*)managedObjectContext {
	if (managedObjectContext != _managedObjectContext) {
		// the database's main databaseObjectContext doesn't retain the database
		if ([managedObjectContext isKindOfClass:[N2ManagedObjectContext class]])
            ((N2ManagedObjectContext*)managedObjectContext).database = nil;
		[self willChangeValueForKey:@"managedObjectContext"];
		[_managedObjectContext release];
		_managedObjectContext = [managedObjectContext retain];
		[self didChangeValueForKey:@"managedObjectContext"];
	}
}

-(NSManagedObjectModel*)managedObjectModel {
	[NSException raise:NSGenericException format:@"[%@ managedObjectModel] must be defined", self.className];
	return NULL;
}

/*-(NSMutableDictionary*)persistentStoreCoordinatorsDictionary {
	static NSMutableDictionary* dict = NULL;
	if (!dict)
		dict = [[NSMutableDictionary alloc] initWithCapacity:4];
	return dict;
}*/

-(BOOL)migratePersistentStoresAutomatically {
	return YES;
}

-(NSManagedObjectContext*)contextAtPath:(NSString*)sqlFilePath {
	sqlFilePath = sqlFilePath.stringByExpandingTildeInPath;
	
    N2ManagedObjectContext* moc = [[[N2ManagedObjectContext alloc] init] autorelease];
    //	NSLog(@"---------- NEW %@ at %@", moc, sqlFilePath);
	moc.undoManager = nil;
	moc.database = self;
	
    //	NSMutableDictionary* persistentStoreCoordinatorsDictionary = self.persistentStoreCoordinatorsDictionary;
	
	@synchronized (self) {
		if ([sqlFilePath isEqualToString:self.sqlFilePath])
			moc.persistentStoreCoordinator = self.managedObjectContext.persistentStoreCoordinator;
		if (!moc.persistentStoreCoordinator) {
            //			moc.persistentStoreCoordinator = [persistentStoreCoordinatorsDictionary objectForKey:sqlFilePath];
			
			BOOL isNewFile = ![NSFileManager.defaultManager fileExistsAtPath:sqlFilePath];
			if (isNewFile)
				moc.persistentStoreCoordinator = nil;
			
			if (!moc.persistentStoreCoordinator) {
				NSPersistentStoreCoordinator* persistentStoreCoordinator = moc.persistentStoreCoordinator = [[[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:self.managedObjectModel] autorelease];
                //				[persistentStoreCoordinatorsDictionary setObject:persistentStoreCoordinator forKey:sqlFilePath];
				
				NSPersistentStore* pStore = nil;
				int i = 0;
				do { // try 2 times
					++i;
					
					NSError* err = NULL;
					NSDictionary* options = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:[self migratePersistentStoresAutomatically]], NSMigratePersistentStoresAutomaticallyOption, [NSNumber numberWithBool:YES], NSInferMappingModelAutomaticallyOption, NULL];
					NSURL* url = [NSURL fileURLWithPath:sqlFilePath];
					@try {
						pStore = [persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:NULL URL:url options:options error:&err];
					} @catch (...) {
					}
					
					if (!pStore && i == 1) {
						NSLog(@"Error: [N2ManagedDatabase contextAtPath:] %@", [err description]);
						NSRunCriticalAlertPanel([NSString stringWithFormat:NSLocalizedString(@"%@ Storage Error", nil), [self className]], err.localizedDescription, NSLocalizedString(@"OK", NULL), NULL, NULL);
						
						// error = [NSError osirixErrorWithCode:0 underlyingError:error localizedDescriptionFormat:NSLocalizedString(@"Store Configuration Failure: %@", NULL), error.localizedDescription? error.localizedDescription : NSLocalizedString(@"Unknown Error", NULL)];
						
						// delete the old file...
						[NSFileManager.defaultManager removeItemAtPath:sqlFilePath error:NULL];
						// [NSFileManager.defaultManager removeItemAtPath: [defaultPortalUsersDatabasePath.stringByExpandingTildeInPath.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"WebUsers.vers"] error:NULL];
					}
				} while (!pStore && i < 2);
			}
			
			if (isNewFile)
				NSLog(@"New database file created at %@", sqlFilePath);
		}
        
        // this line is very important, if there is no sql file
        [moc save:NULL];
	}
    
    return moc;
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
	
    _saveLock = [[NSLock alloc] init];
    
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

-(id)independentDatabase {
	return [[[[self class] alloc] initWithPath:self.sqlFilePath context:[self independentContext]] autorelease];
}

-(NSEntityDescription*)entityForName:(NSString*)name {
	return [NSEntityDescription entityForName:name inManagedObjectContext:self.managedObjectContext];
}

-(id)objectWithID:(NSString*)urlString {
	return [self.managedObjectContext objectWithID:[self.managedObjectContext.persistentStoreCoordinator managedObjectIDForURIRepresentation:[NSURL URLWithString:urlString]]];
}

-(NSArray*)objectsForEntity:(NSEntityDescription*)e {
	return [self objectsForEntity:e predicate:nil error:NULL];
}

-(NSArray*)objectsForEntity:(NSEntityDescription*)e predicate:(NSPredicate*)p {
	return [self objectsForEntity:e predicate:p error:NULL];
}

-(NSArray*)objectsForEntity:(NSEntityDescription*)e predicate:(NSPredicate*)p error:(NSError**)err {
	NSFetchRequest* req = [[[NSFetchRequest alloc] init] autorelease];
	req.entity = e;
	req.predicate = p? p : [NSPredicate predicateWithValue:YES];
    
//    [self.managedObjectContext lock];
    @try {
        return [self.managedObjectContext executeFetchRequest:req error:err];
    } @catch (NSException* e) {
        N2LogException(e);
    } @finally {
//		[self.managedObjectContext unlock];
    }
	return nil;
}

-(NSUInteger)countObjectsForEntity:(NSEntityDescription*)e {
	return [self countObjectsForEntity:e predicate:nil error:NULL];
}

-(NSUInteger)countObjectsForEntity:(NSEntityDescription*)e predicate:(NSPredicate*)p {
	return [self countObjectsForEntity:e predicate:p error:NULL];
}

-(NSUInteger)countObjectsForEntity:(NSEntityDescription*)e predicate:(NSPredicate*)p error:(NSError**)err {
	NSFetchRequest* req = [[[NSFetchRequest alloc] init] autorelease];
	req.entity = e;
	req.predicate = p? p : [NSPredicate predicateWithValue:YES];
    
    [self.managedObjectContext lock];
    @try {
        return [self.managedObjectContext countForFetchRequest:req error:err];
    } @catch (NSException* e) {
        N2LogException(e);
    } @finally {
		[self.managedObjectContext unlock];
    }
    
	return 0;
}

-(id)newObjectForEntity:(NSEntityDescription*)entity {
	return [[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:self.managedObjectContext];
}

-(BOOL)save {
    return [self save:NULL];
}

-(BOOL)save:(NSError**)err {
	NSError* perr = NULL;
	if (!err) err = &perr;
	
	BOOL b = NO;
	
    if ([_saveLock tryLock])
        @try {
            b = [self.managedObjectContext save:err];
        } @catch(NSException* e) {
            if (!*err)
                *err = [NSError errorWithDomain:@"Exception" code:-1 userInfo:[NSDictionary dictionaryWithObject:e forKey:@"Exception"]];
        } @finally {
    		[_saveLock unlock];
        }
	
	return b;
}


@end
