package com.example.airsuspension;

import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
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
}