// Bootstrap ABI adapter for Platform 253. Equivalent to the Apache-2.0 upstream
// platform/starter/src/com/intellij/idea/MainImpl.kt, with our product prefix retained.
package com.intellij.idea;

import com.intellij.ide.bootstrap.InitAppContext;
import com.intellij.platform.ide.bootstrap.AppStarter;
import com.intellij.platform.ide.bootstrap.ApplicationLoader;
import kotlin.Unit;
import kotlin.coroutines.Continuation;

public final class MainImpl implements AppStarter {
    @Override public Object start(InitAppContext context,Continuation<? super Unit> continuation){
        ApplicationLoader.initApplication(context);
        return Unit.INSTANCE;
    }
}
