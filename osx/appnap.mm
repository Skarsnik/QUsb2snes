#import <Foundation/Foundation.h>
#import <Foundation/NSProcessInfo.h>
#include "appnap.h"

AppNapSuspender::AppNapSuspender()
{

}

void AppNapSuspender::suspend()
{
    static id activityId;
    activityId = [[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep reason:@"No naps"];
}


