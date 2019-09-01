#include "interface.h"
#include "error.h"
#include "register.h"

MyOpenFile(FSSpec *myFSS)
{
	if(gRegistered)
	{
		if(gLevelResFile)
			CloseResFile(gLevelResFile);
		gLevelResFile=FSpOpenResFile(myFSS,fsRdPerm);
		BlockMove(myFSS->name,gLevelFileName,myFSS->name[0]+1);
	}
	else{
		AlertStdAlertParamRec alertParam={
		false,false,nil,
		"\pOK",
		nil,
		nil,
		kAlertStdAlertOKButton,
		0,
		kWindowDefaultPosition};
		short alertHit;

		DoError(StandardAlert(kAlertStopAlert,
			"\pYou cannot use custom level files using the unregistered version of Reckless Drivin'.",
			"\pPlease pay the registration fee for Reckless Drivin to get rid of this message.",
			&alertParam,
			&alertHit));
	}

}

OSErr	MyGotRequiredParams (AppleEvent *theAppleEvent)
{
	DescType	returnedType;
	Size	actualSize;
	OSErr	myErr;

	myErr = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr,
					typeWildCard, &returnedType,
					nil, 0, &actualSize);

	if (myErr == errAEDescNotFound)	// you got all the required
											//parameters
		return	noErr;
	else
		if (myErr == noErr)  // you missed a required parameter
			return	errAEParamMissed;
		else	// the call to AEGetAttributePtr failed
			return	myErr;
}


pascal OSErr myODOC(AppleEvent *theAppleEvent,AppleEvent *reply,long refCon)
{
	FSSpec	myFSS;
	AEDescList	docList;
	OSErr	myErr;
	long	index, itemsInList;
	Size	actualSize;
	AEKeyword	keywd;
	DescType	returnedType;

	// get the direct parameter--a descriptor list--and put
	// it into docList
	myErr = AEGetParamDesc(theAppleEvent, keyDirectObject,
							typeAEList, &docList);
	if (myErr)
		return myErr;

	// check for missing required parameters
	myErr = MyGotRequiredParams(theAppleEvent);
	if (myErr) {
		// an error occurred:  do the necessary error handling
		myErr = AEDisposeDesc(&docList);
		return	myErr;
	}

	// count the number of descriptor records in the list
	myErr = AECountItems (&docList,&itemsInList);

	// now get each descriptor record from the list, coerce
	// the returned data to an FSSpec record, and open the
	// associated file
	for (index=1; index<=itemsInList; index++) {
		myErr = AEGetNthPtr(&docList, index, typeFSS, &keywd,
							&returnedType, (Ptr)&myFSS,
							sizeof(myFSS), &actualSize);
		if (myErr)
			DoError(myErr);
		MyOpenFile(&myFSS);
	}

	myErr = AEDisposeDesc(&docList);

	return	noErr;
}

pascal OSErr myOAPP(AppleEvent *theAE,AppleEvent *reply,long refCon)
{
	return noErr;
}

pascal OSErr myPDOC(AppleEvent *theAE,AppleEvent *reply,long refCon)
{
	return noErr;
}

pascal OSErr myQUIT(AppleEvent *theAE,AppleEvent *reply,long refCon)
{
	OSErr	myErr;

	// check for missing required parameters
	myErr = MyGotRequiredParams(theAE);
	if (myErr) {
		// an error occurred:  do the necessary error handling
		return	myErr;
	}
	gExit=true;
	return noErr;
}

void InitAE()
{
	AEInstallEventHandler(kCoreEventClass,kAEOpenApplication,NewAEEventHandlerUPP(&myOAPP),0,false);
	AEInstallEventHandler(kCoreEventClass,kAEOpenDocuments,NewAEEventHandlerUPP(&myODOC),0,false);
	AEInstallEventHandler(kCoreEventClass,kAEPrintDocuments,NewAEEventHandlerUPP(&myPDOC),0,false);
	AEInstallEventHandler(kCoreEventClass,kAEQuitApplication,NewAEEventHandlerUPP(&myQUIT),0,false);
}