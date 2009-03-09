/*=========================================================================
  Program:   OsiriX

  Copyright (c) OsiriX Team
  All rights reserved.
  Distributed under GNU - GPL
  
  See http://www.osirix-viewer.com/copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.
=========================================================================*/



#import <AppKit/AppKit.h>
#import "DCMPix.h"

#ifdef __cplusplus
#import "VTKView.h"

#define id Id
#include "vtkCommand.h"
#include "vtkProperty.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume16Reader.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkOutlineFilter.h"
#include "vtkImageReader.h"
#include "vtkImageImport.h"
#include "vtkCamera.h"
#include "vtkStripper.h"
#include "vtkLookupTable.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkProperty.h"
#include "vtkPolyDataNormals.h"
#include "vtkContourFilter.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkImageActor.h"
#include "vtkLight.h"

#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPlaneSource.h"
#include "vtkBoxWidget.h"
#include "vtkPlaneWidget.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolumeRayCastMIPFunction.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkTransform.h"
#include "vtkSphere.h"
#include "vtkImplicitBoolean.h"
#include "vtkExtractGeometry.h"
#include "vtkDataSetMapper.h"
#include "vtkPicker.h"
#include "vtkCellPicker.h"
#include "vtkPointPicker.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkExtractPolyDataGeometry.h"
#include "vtkProbeFilter.h"
#include "vtkCutter.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkXYPlotActor.h"
#include "vtkClipPolyData.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkImageFlip.h"
#include "vtkAnnotatedCubeActor.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkVolumeTextureMapper2D.h"
#include "vtkVolumeTextureMapper3D.h"
#include "OsiriXFixedPointVolumeRayCastMapper.h"

#include "vtkCellArray.h"
#include "vtkProperty2D.h"

#undef id

class vtkMyCallbackVR;

#else

typedef char* vtkTransform;
typedef char* vtkImageActor;
typedef char* vtkImageMapToColors;
typedef char* vtkLookupTable;
typedef char* vtkImageReslice;
typedef char* vtkImageImport;
typedef char* vtkCamera;
typedef char* vtkActor;
typedef char* vtkPolyDataMapper;
typedef char* vtkOutlineFilter;
typedef char* vtkLineWidget;

typedef char* vtkTextActor;
typedef char* vtkVolumeRayCastMapper;
typedef char* vtkFixedPointVolumeRayCastMapper;
typedef char* OsiriXFixedPointVolumeRayCastMapper;
typedef char* vtkVolumeRayCastMIPFunction;
typedef char* vtkVolume;


typedef char* vtkPiecewiseFunction;
typedef char* vtkVolumeTextureMapper2D;
typedef char* vtkPolyData;
typedef char* vtkVolumeProperty;
typedef char* vtkPolyDataMapper2D;

typedef char* vtkColorTransferFunction;
typedef char* vtkActor2D;
typedef char* vtkMyCallback;
typedef char* vtkPlaneWidget;
typedef char* vtkBoxWidget;
typedef char* vtkVolumeRayCastCompositeFunction;

typedef char* vtkRenderer;
typedef char* vtkVolumeTextureMapper3D;
typedef char* vtkOrientationMarkerWidget;

typedef char* vtkMyCallbackVR;
#endif

#include <Accelerate/Accelerate.h>
#import "ViewerController.h"
#import "WaitRendering.h"

#import "Schedulable.h"
#import "Scheduler.h"
#import "StaticScheduler.h"

@class DICOMExport;
@class Camera;
@class VRController;
@class OSIVoxel;

#import "CLUTOpacityView.h"

/** \brief  Volume Rendering View
*
*   View for volume rendering and MIP
*/

#ifdef __cplusplus
@interface VRView : VTKView <Schedulable>
#else
@interface VRView : NSView
#endif
{
	NSTimer						*autoRotate, *startAutoRotate;
	BOOL						rotate, flyto;
	int							incFlyTo;
	
	float						flyToDestination[ 3];

	int							projectionMode;
    NSMutableArray				*blendingPixList;
    DCMPix						*blendingFirstObject;
    float						*blendingData, blendingFactor;
	ViewerController			*blendingController;
	char						*blendingData8;
	vImage_Buffer				blendingSrcf, blendingDst8;
	float						blendingWl, blendingWw, measureLength;
	vtkImageImport				*blendingReader;
	
	OsiriXFixedPointVolumeRayCastMapper *blendingVolumeMapper;
	vtkVolumeTextureMapper3D	*blendingTextureMapper;
	
	vtkVolume					*blendingVolume;
	vtkVolumeProperty			*blendingVolumeProperty;
	vtkColorTransferFunction	*blendingColorTransferFunction;
	vtkVolumeRayCastCompositeFunction *blendingCompositeFunction;
	vtkPiecewiseFunction		*blendingOpacityTransferFunction;
	double						blendingtable[256][3];
	
	BOOL						needToFlip, blendingNeedToFlip, firstTime;
	
	IBOutlet NSWindow			*export3DWindow;
	IBOutlet NSSlider			*framesSlider;
	IBOutlet NSMatrix			*quality, *rotation, *orientation;
	IBOutlet NSTextField		*pixelInformation;

	IBOutlet NSWindow			*exportDCMWindow;
	IBOutlet NSSlider			*dcmframesSlider;
	IBOutlet NSMatrix			*dcmExportMode, *dcmquality, *dcmrotation, *dcmorientation;
	IBOutlet NSBox				*dcmBox;
	IBOutlet NSTextField		*dcmSeriesName;
	IBOutlet NSMatrix			*dcmExportDepth;

	IBOutlet NSWindow       *export3DVRWindow;
	IBOutlet NSMatrix		*VRFrames;
	IBOutlet NSMatrix		*VRquality;
	
	IBOutlet NSMatrix		*scissorStateMatrix;
	IBOutlet NSColorWell	*backgroundColor;
	
	IBOutlet NSObjectController	*shadingController;
	
	long					numberOfFrames;
	BOOL					bestRenderingMode;
	float					rotationValue, factor;
	long					rotationOrientation, renderingMode;
	
	NSArray					*currentOpacityArray;
    NSMutableArray			*pixList;
    DCMPix					*firstObject;
    float					*data;
	
	float					valueFactor, blendingValueFactor;
	float					OFFSET16, blendingOFFSET16;
	
	unsigned char			*dataFRGB;
	char					*data8;
	vImage_Buffer			srcf, dst8;

    short					currentTool;
	float					wl, ww;
	float					LOD, lowResLODFactor;
	float					cosines[ 9];
	float					blendingcosines[ 9];
	double					table[256][3];
	double					alpha[ 256];

	NSCursor				*cursor;
	BOOL					cursorSet;
	
    vtkRenderer				*aRenderer;
    vtkCamera				*aCamera;

    vtkActor				*outlineRect;
    vtkPolyDataMapper		*mapOutline;
    vtkOutlineFilter		*outlineData;
	
	vtkMyCallbackVR				*cropcallback;
	vtkPlaneWidget				*planeWidget;
	vtkOrientationMarkerWidget	*orientationWidget;
	vtkBoxWidget				*croppingBox;
	double						initialCroppingBoxBounds[6];
	// MAPPERS
	
	OsiriXFixedPointVolumeRayCastMapper *volumeMapper;
	vtkVolumeTextureMapper3D		*textureMapper;
	
	vtkVolume					*volume;
	vtkVolumeProperty			*volumeProperty;
	vtkColorTransferFunction	*colorTransferFunction;
	vtkTextActor				*textWLWW, *textX;
	BOOL						isViewportResizable;
	vtkTextActor				*oText[ 4], oTextS[ 4];
	char						WLWWString[ 200];
	vtkImageImport				*reader;
	vtkVolumeRayCastCompositeFunction  *compositeFunction;
	vtkPiecewiseFunction		*opacityTransferFunction;
	
	vtkColorTransferFunction	*red, *green, *blue;
	BOOL						noWaitDialog, isRGB, isBlendingRGB, ROIUPDATE;
	WaitRendering				*splash;
	
	double						camPosition[ 3], camFocal[ 3];
	
	NSMutableArray				*ROIPoints;
	
	vtkPolyData					*ROI3DData;
	vtkPolyDataMapper2D			*ROI3D;
	vtkActor2D					*ROI3DActor;
	
	vtkPolyData					*Line2DData;
	vtkPolyDataMapper2D			*Line2D;
	vtkActor2D					*Line2DActor;
	vtkTextActor				*Line2DText;
	
	BOOL						clamping;
	
	DICOMExport					*exportDCM;
	
	NSMutableArray				*point3DActorArray;
	NSMutableArray				*point3DPositionsArray;
	NSMutableArray				*point3DRadiusArray;
	NSMutableArray				*point3DColorsArray;
	BOOL						display3DPoints;
	IBOutlet NSPanel			*point3DInfoPanel;
	IBOutlet NSSlider			*point3DRadiusSlider;
	IBOutlet NSColorWell		*point3DColorWell;
	IBOutlet NSButton			*point3DPropagateToAll, *point3DSetDefault;
	IBOutlet VRController		*controller;
	float						point3DDefaultRadius, point3DDefaultColorRed, point3DDefaultColorGreen, point3DDefaultColorBlue, point3DDefaultColorAlpha;
	
	BOOL						_dragInProgress;
	NSTimer						*_mouseDownTimer, *_rightMouseDownTimer;
	NSImage						*destinationImage;
	
	NSPoint						_mouseLocStart, _previousLoc;  // mouseDown start point
	BOOL						_resizeFrame;
	short						_tool;
	
	float						_startWW, _startWL, _startMin, _startMax;
	
	NSRect						savedViewSizeFrame;
	
	float						firstPixel, secondPixel;
	
	NSLock						*deleteRegion;
	
	IBOutlet CLUTOpacityView	*clutOpacityView;
	BOOL						advancedCLUT;
	NSData						*appliedCurves;
	BOOL						appliedResolution;
	BOOL						gDataValuesChanged;

	float						verticalAngleForVR;
	float						rotateDirectionForVR;
	
	BOOL						_contextualMenuActive;
	
	//Context for rendering to iChat
	BOOL						_hasChanged;
	float						iChatWidth, iChatHeight;
	BOOL						iChatFrameIsSet;
	
	// 3DConnexion SpaceNavigator
	NSTimer			*snCloseEventTimer;
	BOOL			snStopped;
	UInt16			snConnexionClientID;
	
	BOOL			clipRangeActivated;
	double			clippingRangeThickness;
	
	BOOL			bestRenderingWasGenerated;
	float superSampling;
	BOOL dontResetImage, keep3DRotateCentered;
}

@property BOOL clipRangeActivated, keep3DRotateCentered, dontResetImage;
@property int projectionMode;
@property double clippingRangeThickness;
@property long renderingMode;
@property (readonly) NSArray* currentOpacityArray;

+ (BOOL) getCroppingBox:(double*) a :(vtkVolume *) volume :(vtkBoxWidget*) croppingBox;
+ (void) setCroppingBox:(double*) a :(vtkVolume *) volume;
- (void) setCroppingBox:(double*) a;
- (BOOL) croppingBox:(double*) a;
- (void) showCropCube:(id) sender;
- (void) restoreFullDepthCapture;
- (void) prepareFullDepthCapture;
- (float*) imageInFullDepthWidth: (long*) w height:(long*) h isRGB:(BOOL*) isRGB;
- (NSDictionary*) exportDCMCurrentImage;
- (NSDictionary*) exportDCMCurrentImageIn16bit: (BOOL) fullDepth;
- (void) renderImageWithBestQuality: (BOOL) best waitDialog: (BOOL) wait;
- (void) renderImageWithBestQuality: (BOOL) best waitDialog: (BOOL) wait display: (BOOL) display;
- (void) endRenderImageWithBestQuality;
- (void) resetAutorotate:(id) sender;
- (void) setEngine: (long) engineID showWait:(BOOL) showWait;
- (IBAction)changeColorWith:(NSColor*) color;
- (IBAction)changeColor:(id)sender;
- (NSColor*)backgroundColor;
- (void) exportDICOMFile:(id) sender;
-(unsigned char*) getRawPixels:(long*) width :(long*) height :(long*) spp :(long*) bpp :(BOOL) screenCapture :(BOOL) force8bits;
-(unsigned char*) getRawPixels:(long*) width :(long*) height :(long*) spp :(long*) bpp :(BOOL) screenCapture :(BOOL) force8bits offset:(int*) offset isSigned:(BOOL*) isSigned;
-(void) set3DStateDictionary:(NSDictionary*) dict;
-(NSMutableDictionary*) get3DStateDictionary;
- (void) setBlendingEngine: (long) engineID;
- (void) getShadingValues:(float*) ambient :(float*) diffuse :(float*) specular :(float*) specularpower;
- (void) setShadingValues:(float) ambient :(float) diffuse :(float) specular :(float) specularpower;
-(void) movieChangeSource:(float*) volumeData;
-(void) movieChangeSource:(float*) volumeData showWait :(BOOL) showWait;
-(void) movieBlendingChangeSource:(long) index;
-(void) setBlendingWLWW:(float) iwl :(float) iww;
-(void) setBlendingCLUT:( unsigned char*) r : (unsigned char*) g : (unsigned char*) b;
-(void) setBlendingFactor:(float) a;
//-(NSDate*) startRenderingTime;
//-(void) newStartRenderingTime;
//-(void) deleteStartRenderingTime;
-(void) setOpacity:(NSArray*) array;
- (void) setLowResolutionCamera: (Camera*) cam;
//-(void) runRendering;
//-(void) startRendering;
//-(void) stopRendering;
-(void) setLOD:(float)f;
-(void) setCurrentTool:(short) i;
- (int) currentTool;
- (int) _tool;
-(id)initWithFrame:(NSRect)frame;
-(short)setPixSource:(NSMutableArray*)pix :(float*) volumeData;
-(void)dealloc;
//Fly to point in world coordinates;
- (void) flyTo:(float) x :(float) y :(float) z;
// Fly to Volume Point 
- (void) flyToVoxel:(OSIVoxel *)voxel;
//Fly to 2D position on a slice;
- (void) flyToPoint:(NSPoint)point  slice:(int)slice;
- (void) processFlyTo;
-(void) setWLWW:(float) wl :(float) ww;
-(void) getWLWW:(float*) wl :(float*) ww;
-(void) setBlendingPixSource:(ViewerController*) bC;
-(IBAction) endQuicktimeSettings:(id) sender;
-(IBAction) endDCMExportSettings:(id) sender;
-(IBAction) endQuicktimeVRSettings:(id) sender;
-(IBAction) exportQuicktime :(id) sender;
-(float) rotation;
-(float) numberOfFrames;
-(void) Azimuth:(float) z;
-(void) Vertical:(float) z;
-(NSImage*) nsimageQuicktime;
-(NSImage*) nsimage:(BOOL) q;
-(void) setCLUT:( unsigned char*) r : (unsigned char*) g : (unsigned char*) b;
-(void)activateShading:(BOOL)on;
-(IBAction) switchShading:(id) sender;
-(long) shading;
- (void) setEngine: (long) engineID;
- (void) setProjectionMode: (int) mode;
- (IBAction) resetImage:(id) sender;
-(void) saView:(id) sender;
- (IBAction)setRenderMode:(id)sender;
- (void) setBlendingMode: (long) modeID;
-(NSImage*) nsimageQuicktime:(BOOL) renderingMode;
- (vtkRenderer*) vtkRenderer;
- (vtkCamera*) vtkCamera;
- (void) setVtkCamera:(vtkCamera*)aVtkCamera;
- (void)setCenterlineCamera: (Camera *) cam;
- (void) setCamera: (Camera*) cam;
- (Camera*) camera;
- (Camera*) cameraWithThumbnail:(BOOL) produceThumbnail;
- (IBAction) scissorStateButtons:(id) sender;
- (void) updateScissorStateButtons;
-(void) switchOrientationWidget:(id) sender;
- (void) computeOrientationText;
- (void) getOrientation: (float*) o;
-(void) bestRendering:(id) sender;
- (void) setMode: (long) modeID;
- (long) mode;
- (double) getResolution;
- (void) getCosMatrix: (float *) cos;
- (void) getOrigin: (float *) origin;
- (void) getOrigin: (float *) origin windowCentered:(BOOL) wc;
- (void) getOrigin: (float *) origin windowCentered:(BOOL) wc sliceMiddle:(BOOL) sliceMiddle;
- (BOOL) isViewportResizable;
- (void) setViewportResizable: (BOOL) boo;

// 3D Points
- (BOOL) get3DPixelUnder2DPositionX:(float) x Y:(float) y pixel: (long*) pix position:(float*) position value:(float*) val;
- (BOOL) get3DPixelUnder2DPositionX:(float) x Y:(float) y pixel: (long*) pix position:(float*) position value:(float*) val maxOpacity: (float) maxOpacity minValue: (float) minValue;

- (void) add3DPoint: (double) x : (double) y : (double) z : (float) radius : (float) r : (float) g : (float) b;
- (void) add3DPoint: (double) x : (double) y : (double) z;
- (void) add3DPointActor: (vtkActor*) actor;
- (void) addRandomPoints: (int) n : (int) r;
- (void) throw3DPointOnSurface: (double) x : (double) y;
- (void) setDisplay3DPoints: (BOOL) on;
- (void) toggleDisplay3DPoints;
- (BOOL) isAny3DPointSelected;
- (unsigned int) selected3DPointIndex;
- (void) unselectAllActors;
- (void) remove3DPointAtIndex: (unsigned int) index;
- (void) removeSelected3DPoint;
- (IBAction) IBSetSelected3DPointColor: (id) sender;
- (IBAction) IBSetSelected3DPointRadius: (id) sender;
- (IBAction) IBPropagate3DPointsSettings: (id) sender;
- (void) setSelected3DPointColor: (NSColor*) color;
- (void) setAll3DPointsColor: (NSColor*) color;
- (void) set3DPointAtIndex:(unsigned int) index Color: (NSColor*) color;
- (void) setSelected3DPointRadius: (float) radius;
- (void) setAll3DPointsRadius: (float) radius;
- (void) set3DPointAtIndex:(unsigned int) index Radius: (float) radius;
- (IBAction) save3DPointsDefaultProperties: (id) sender;
- (void) load3DPointsDefaultProperties;
- (void) convert3Dto2Dpoint:(double*) pt3D :(double*) pt2D;
- (void)convert2DPoint:(float *)pt2D to3DPoint:(float *)pt3D;
- (IBAction) setCurrentdcmExport:(id) sender;
- (IBAction) switchToSeriesRadio:(id) sender;
- (float) offset;
- (float) valueFactor;
- (void) setViewportResizable: (BOOL) boo;
- (void) squareView:(id) sender;
- (void) computeValueFactor;
- (void) setRotate: (BOOL) r;
- (float) factor;
- (float) imageSampleDistance;
-(void) setViewSizeToMatrix3DExport;
-(void) restoreViewSizeAfterMatrix3DExport;
-(void) axView:(id) sender;
-(void) coView:(id) sender;
-(void) saViewOpposite:(id) sender;
- (void) render;
- (void)zoomMouseUp:(NSEvent *)theEvent;
- (void) setWindowCenter: (NSPoint) loc;
- (NSPoint) windowCenter;
- (double) getClippingRangeThicknessInMm;
- (void) setLODLow:(BOOL) l;

// export
- (void) sendMail:(id) sender;
- (void) exportJPEG:(id) sender;
- (void) export2iPhoto:(id) sender;
- (void) exportTIFF:(id) sender;

// cursors
-(void) setCursorForView: (long) tool;

//Dragging
- (void) startDrag:(NSTimer*)theTimer;
- (void)deleteMouseDownTimer;

//Menus
- (void)deleteRightMouseDownTimer;
- (void) showMenu:(NSTimer*)theTimer;

-(BOOL)actionForHotKey:(NSString *)hotKey;
- (void)setAdvancedCLUT:(NSMutableDictionary*)clut lowResolution:(BOOL)lowRes;
- (void)setAdvancedCLUTWithName:(NSString*)name;
- (BOOL)advancedCLUT;
- (VRController*)controller;
- (void)setController:(VRController*)aController;
- (BOOL)isRGB;

- (OsiriXFixedPointVolumeRayCastMapper*)volumeMapper;
- (void)setVolumeMapper:(OsiriXFixedPointVolumeRayCastMapper*)aVolumeMapper;
- (vtkVolume*)volume;
- (void)setVolume:(vtkVolume*)aVolume;
- (char*)data8;
- (void)setData8:(char*)data;

- (void)drawImage:(NSImage *)image inBounds:(NSRect)rect;
- (BOOL)checkHasChanged;
- (void)setIChatFrame:(BOOL)boo;
- (void)_iChatStateChanged:(NSNotification *)aNotification;

- (void)yaw:(float)degrees;
- (void)panX:(double)x Y:(double)y;

- (void)recordFlyThru;

// 3DConnexion SpaceNavigator
- (void)connect2SpaceNavigator;
void VRSpaceNavigatorMessageHandler(io_connect_t connection, natural_t messageType, void *messageArgument);

@end
