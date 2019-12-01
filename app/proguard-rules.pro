-optimizationpasses 3
-overloadaggressively
-assumenosideeffects class kotlin.jvm.internal.Intrinsics {
    public static *** checkReturnedValueIsNotNull(...);
    public static *** checkParameterIsNotNull(...);
    public static *** checkExpressionValueIsNotNull(...);
    public static *** throwNpe(...);
    public static *** throwUninitializedPropertyAccessException(...);
}
#-repackageclasses ''
-allowaccessmodification
-keep class ru.ostrovskal.zx.forms.*
#-keep class ru.ostrovskal.droid.tables.***
-dontobfuscate
#-dontshrink
#-keepattributes *Annotation*,SourceFile,LineNumberTable
