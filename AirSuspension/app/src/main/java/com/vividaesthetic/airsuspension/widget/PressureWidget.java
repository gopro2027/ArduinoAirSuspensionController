package com.vividaesthetic.airsuspension.widget;

import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.widget.RemoteViews;

import com.vividaesthetic.airsuspension.R;

/**
 * Implementation of App Widget functionality.
 */
public class PressureWidget extends AppWidgetProvider {

    static void updateAppWidget(Context context, AppWidgetManager appWidgetManager,
                                int appWidgetId) {

        // Construct the RemoteViews object
        RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.pressure_widget);

        //views.setTextViewText(R.id.pressure_fd, text);

        // Instruct the widget manager to update the widget
        appWidgetManager.updateAppWidget(appWidgetId, views);
    }

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
        //if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.S) {
            // There may be multiple widgets active, so update all of them
            for (int appWidgetId : appWidgetIds) {
                updateAppWidget(context, appWidgetManager, appWidgetId);
            }
        /*} else {
            for (int appWidgetId : appWidgetIds) {

                RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.pressure_widget);

                appWidgetManager.updateAppWidget(appWidgetId, views);
            }
        }*/
    }

    @Override
    public void onEnabled(Context context) {
        // Enter relevant functionality for when the first widget is created
    }

    @Override
    public void onDisabled(Context context) {
        // Enter relevant functionality for when the last widget is disabled
    }
}