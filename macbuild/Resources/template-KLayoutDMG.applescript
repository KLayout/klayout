-------------------------------------------------------------------------------------------------
(*
 * Template File:
 *  macbuild/Resources/template-KLayoutDMG.applescript
 *
 * On the runtime, "makeDMG4mac.py" will generate the actual AppleScrip File
 *    "macbuild/Resources/KLayoutDMG.applescript"
 * from this template. Hence, the generated actual scrip file does not need
 * to be version controlled by Git.
 *
 * Description:
 *  This is a template AppleScript to make a fancy DMG installer of KLayout
 *  (http://www.klayout.de/index.php) application bundles.
 *  "makeDMG4mac.py" will read this template and generate the actual AppleScript to execute.
 *  Those values to be found and replaced by "makeDMG4mac.py" are marked by ${KEYWORD} in this
 *  template file.
 *
 *  The background PNG image file (1000 x 500 pix size) was designed by using Logoist3
 *  (http://www.syniumsoftware.com/en/logoist).
 *-----------------------------------------------------------------------------------------------
 *  This is a derivative work of Ref. 2) below. Refer to "macbuild/LICENSE" file.
 *  Ref.
 *   1) https://el-tramo.be/guides/fancy-dmg/
 *   2) https://github.com/andreyvit/create-dmg.git
 *)
-------------------------------------------------------------------------------------------------
on run (volumeName) -- most likely, the volume name is "KLayout"
    tell application "Finder"
        repeat 20 times
            if (exists (disk (volumeName as string))) then
                exit repeat
            end if
            delay 1
        end repeat
        tell disk (volumeName as string)
            -- [1] Open the volume
            open

            -- [2] Set the key coordinates and windows size
            --     The size of given background PNG image is 1000 x 500 pix
            --     ORGX       =   [50] pix
            --     ORGY       =  [100] pix
            --     WIN_WIDTH  = [1000] pix
            --     WIN_HEIGHT =  [500] pix
            set posMargin   to 50
            set negMargin   to 10
            set theTopLeftX to ${ORGX}
            set theTopLeftY to ${ORGY}
            set theWidth    to ${WIN_WIDTH}
            set theHeight   to ${WIN_HEIGHT}
            set theBottomRightX to (theTopLeftX + theWidth  + posMargin)
            set theBottomRightY to (theTopLeftY + theHeight + posMargin)

            -- [3] Set the full path to .DS_Store file
            set dotDSStore to "${FULL_PATH_DS_STORE}"

            -- [4] Set global view options
            tell container window
                set current view to icon view
                set toolbar visible to false
                set statusbar visible to false
                set statusbar visible to false
                set bounds to {theTopLeftX, theTopLeftY, theBottomRightX, theBottomRightY}
                set position of every item to {theTopLeftX + 150, theTopLeftY + 350}
            end tell

            -- [5] Set icon view options
            set opts to the icon view options of container window
            tell opts
                set icon size to 80
                set text size to 16
                set arrangement to not arranged
            end tell

            -- [6] Set the background PNG image file name stored
            set background picture of opts to file ".background:${BACKGROUND_PNG_FILE}"

            -- [7] Set positions of each icon
            --     ITEM_1 = klayout.app      {860, 165}
            --     ITEM_2 = Applications     {860, 345}
            --     ITEM_3 = AUX. holder      {700, 450}
            set position of item "${ITEM_1}" to {${X1}, ${Y1}}
            set position of item "${ITEM_2}" to {${X2}, ${Y2}}
            ${ITEM_3}

            -- [8] Update the contents of container
            close
            open
            update without registering applications

            -- [9] Force save the negatively resized window size
            delay 2
            tell container window
                set statusbar visible to false
                set bounds to {theTopLeftX, theTopLeftY, theBottomRightX - negMargin, theBottomRightY - negMargin}
            end tell
            update without registering applications
        end tell

        -- [10] Restore back the original window size
        delay 2
        tell disk (volumeName as string)
            tell container window
                set statusbar visible to false
                set bounds to {theTopLeftX, theTopLeftY, theBottomRightX, theBottomRightY}
            end tell
            update without registering applications
        end tell

        -- [11] Wait for some time so that "Finder" can complete writing to .DS_Store file
        delay 3
        set elapsedTime to 0
        set ejected to false
        repeat while ejected is false
            delay 1
            set elapsedTime to elapsedTime + 1
            if (do shell script "[ -f " & dotDSStore & " ]; echo $?") = "0" then set ejected to true
        end repeat
        log "### Elapsed <" & elapsedTime & "> [sec] for writing .DS_Store file."
    end tell
end run

--
-- End of file
--
