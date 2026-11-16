/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of linphone-android
 * (see https://www.linphone.org).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
package org.linphone.utils

import android.content.Context
import android.content.res.Configuration
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.ImageDecoder
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.PorterDuff
import android.graphics.PorterDuffXfermode
import android.graphics.Rect
import android.media.ExifInterface
import android.net.Uri
import android.os.Build
import androidx.annotation.AnyThread
import androidx.annotation.RequiresApi
import androidx.annotation.WorkerThread
import java.io.FileNotFoundException
import org.linphone.contacts.AvatarGenerator
import org.linphone.core.tools.Log
import androidx.core.net.toUri
import androidx.core.graphics.createBitmap
import org.linphone.R

class ImageUtils {
    companion object {
        private const val TAG = "[Image Utils]"

        @AnyThread
        fun generatedAvatarIfNeededAndReturnPath(context: Context, initials: String): String {
            val darkMode = context.resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK == Configuration.UI_MODE_NIGHT_YES
            val suffix = if (darkMode) "_dark" else "_light"

            val generatedAvatarPath = FileUtils.getFileStorageCacheDir("$initials$suffix.png", overrideExisting = true)
            if (generatedAvatarPath.exists()) {
                val path = generatedAvatarPath.absolutePath
                return path
            }

            val builder = AvatarGenerator(context)
            builder.setInitials(initials)
            builder.setAvatarSize(AppUtils.getDimension(R.dimen.avatar_big_size).toInt())
            builder.setTextSize(AppUtils.getDimension(R.dimen.avatar_initials_call_text_size))

            val bitmap = builder.buildBitmap(false)
            val path = FileUtils.storeBitmap(bitmap, generatedAvatarPath)
            return path
        }

        @WorkerThread
        fun getBitmap(
            context: Context,
            path: String?,
            round: Boolean = false
        ): Bitmap? {
            Log.d("$TAG Trying to create Bitmap from path [$path]")
            if (path != null) {
                try {
                    val fromPictureUri = path.toUri()
                    val bitmap = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                        decodeModernBitmap(context, fromPictureUri)
                    } else {
                        decodeLegacyBitmap(context, fromPictureUri)
                    } ?: return null
                    return if (round) {
                        getRoundBitmap(bitmap)
                    } else {
                        bitmap
                    }
                } catch (fnfe: FileNotFoundException) {
                    Log.e("$TAG File [$path] not found: $fnfe")
                    return null
                } catch (e: Exception) {
                    Log.e("$TAG Failed to get bitmap using path [$path]: $e")
                    return null
                }
            }

            Log.e("$TAG Can't get bitmap from null URI")
            return null
        }

        @RequiresApi(28)
        private fun decodeModernBitmap(context: Context, uri: Uri): Bitmap {
            // Shortcuts require software pixels, even if ImageDecoder used a
            // hardware bitmap internally. Keep the existing modern behavior.
            return ImageDecoder.decodeBitmap(
                ImageDecoder.createSource(context.contentResolver, uri)
            ).copy(Bitmap.Config.ARGB_8888, true)
        }

        private fun decodeLegacyBitmap(context: Context, uri: Uri): Bitmap? {
            val bitmap = context.contentResolver.openInputStream(uri)?.use { stream ->
                BitmapFactory.decodeStream(stream)
            } ?: return null

            // ImageDecoder applies camera EXIF orientation automatically.
            // BitmapFactory on Android 7/8 needs the equivalent transform.
            val orientation = try {
                context.contentResolver.openInputStream(uri)?.use { stream ->
                    ExifInterface(stream).getAttributeInt(
                        ExifInterface.TAG_ORIENTATION,
                        ExifInterface.ORIENTATION_NORMAL
                    )
                } ?: ExifInterface.ORIENTATION_NORMAL
            } catch (e: Exception) {
                // PNGs, generated avatars and other formats may have no EXIF.
                Log.d("$TAG Image has no readable EXIF orientation: $e")
                ExifInterface.ORIENTATION_NORMAL
            }
            val transform = MyTelLegacyCompatibility.bitmapOrientation(orientation)
            if (transform == MyTelImageOrientation()) return bitmap

            val matrix = Matrix().apply {
                postRotate(transform.rotationDegrees.toFloat())
                if (transform.mirrorHorizontally) postScale(-1f, 1f)
            }
            return Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
        }

        @AnyThread
        private fun getRoundBitmap(bitmap: Bitmap): Bitmap {
            val output = createBitmap(bitmap.width, bitmap.height)
            val canvas = Canvas(output)
            val color = -0xbdbdbe
            val paint = Paint()
            val rect =
                Rect(0, 0, bitmap.width, bitmap.height)
            paint.isAntiAlias = true
            canvas.drawARGB(0, 0, 0, 0)
            paint.color = color
            canvas.drawCircle(
                bitmap.width / 2.toFloat(),
                bitmap.height / 2.toFloat(),
                bitmap.width / 2.toFloat(),
                paint
            )
            paint.xfermode = PorterDuffXfermode(PorterDuff.Mode.SRC_IN)
            canvas.drawBitmap(bitmap, rect, rect, paint)
            return output
        }
    }
}
