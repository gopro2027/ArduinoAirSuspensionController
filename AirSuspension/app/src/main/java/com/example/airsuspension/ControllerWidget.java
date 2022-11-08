package com.example.airsuspension;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.ArrayMap;
import android.util.Log;
import android.util.SizeF;
import android.widget.RemoteViews;

import java.util.Map;

/**
 * Implementation of App Widget functionality.
 */
public class ControllerWidget extends AppWidgetProvider {

    static void updateAppWidget(Context context, AppWidgetManager appWidgetManager,
                                int appWidgetId) {

        // Construct the RemoteViews object
        RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.controller_widget_wide);

        // Instruct the widget manager to update the widget
        appWidgetManager.updateAppWidget(appWidgetId, views);
    }

    public static final String SYNC_AIRUP    = "automaticWidgetSyncButtonClick_airup";
    public static final String SYNC_AIROUT    = "automaticWidgetSyncButtonClick_airout";

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {

        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.S) {
            // There may be multiple widgets active, so update all of them
            for (int appWidgetId : appWidgetIds) {
                updateAppWidget(context, appWidgetManager, appWidgetId);
            }
        } else {
            for (int appWidgetId : appWidgetIds) {
                RemoteViews wideView =
                        new RemoteViews(context.getPackageName(), R.layout.controller_widget_wide);
                RemoteViews tallView =
                        new RemoteViews(context.getPackageName(), R.layout.controller_widget_tall);

                wideView.setOnClickPendingIntent(R.id.button_airup_w, getPendingSelfIntent(context, SYNC_AIRUP));
                wideView.setOnClickPendingIntent(R.id.button_airout_w, getPendingSelfIntent(context, SYNC_AIROUT));

                tallView.setOnClickPendingIntent(R.id.button_airup_w, getPendingSelfIntent(context, SYNC_AIRUP));
                tallView.setOnClickPendingIntent(R.id.button_airout_w, getPendingSelfIntent(context, SYNC_AIROUT));


                Map<SizeF, RemoteViews> viewMapping = new ArrayMap<>();
                viewMapping.put(new SizeF(200f, 0f), wideView);
                viewMapping.put(new SizeF(0f, 0f), tallView);
                viewMapping.put(new SizeF(0f, 250f), tallView);

                RemoteViews remoteViews = new RemoteViews(viewMapping);
                appWidgetManager.updateAppWidget(appWidgetId, remoteViews);


            }
        }
    }

    @Override
    public void onEnabled(Context context) {
        // Enter relevant functionality for when the first widget is created
    }

    @Override
    public void onDisabled(Context context) {
        // Enter relevant functionality for when the last widget is disabled
    }

    private int calcWidth(int n) {
        return (73*n - 16);
    }
    private int calcHeight(int m) {
        return (118*m - 16);
    }

    protected PendingIntent getPendingSelfIntent(Context context, String action) {
        Intent intent = new Intent(context, getClass());
        intent.setAction(action);
        return PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_IMMUTABLE);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        // TODO Auto-generated method stub
        super.onReceive(context, intent);

        Log.i("widget","got pending intent");

        if (ControllerWidget.SYNC_AIRUP.equals(intent.getAction())) {

            MainActivity.getAirSuspensionController(null).airUp();

            if (context instanceof MainActivity) {
                Log.i("airup","context is part of main activ");
            } else {
                Log.i("airup","context is NOT");
            }



        }

        if (SYNC_AIROUT.equals(intent.getAction())) {

            MainActivity.getAirSuspensionController(null).airOut();

            if (context instanceof MainActivity) {
                Log.i("airout","context is part of main activ");
            } else {
                Log.i("airout","context is NOT");
            }

        }
    }

}