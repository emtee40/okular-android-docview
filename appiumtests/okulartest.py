#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024 Antoine Herlicq <antoine.herlicq@enioka.com>

import os
import tempfile
import time
import unittest
import shutil

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver import ActionChains, Keys
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC


class OkularTests(unittest.TestCase):
    def find_element_by_class_name(self, type, name):
        return self.driver.find_element(by=AppiumBy.CLASS_NAME, value=f"[{type} | {name}]")

    def find_element_by_name(self, element):
        return self.driver.find_element(by=AppiumBy.NAME, value=element)

    def find_element_by_accessibility_id(self, element):
        return self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value=element)

    def write_text(self, area_to_write, text):
        actions = ActionChains(self.driver)
        actions.move_to_element(area_to_write)
        actions.click()
        actions.send_keys(text)
        actions.perform()

    def rotate_left(self):
        self.find_element_by_class_name("menu item", "View").click()
        self.find_element_by_class_name("menu item", "Orientation").click()
        self.find_element_by_class_name("menu item", "Rotate Left").click()

    def rotate_right(self):
        self.find_element_by_class_name("menu item", "View").click()
        self.find_element_by_class_name("menu item", "Orientation").click()
        self.find_element_by_class_name("menu item", "Rotate Right").click()

    def open_view_menu(self):
        self.find_element_by_class_name("menu item", "View").click()

    def open_file_menu(self):
        self.find_element_by_class_name("menu item", "File").click()

    def assert_current_page_has_text(self, text):
        page_number = self.find_element_by_accessibility_id("PageLabelEdit").text
        ActionChains(self.driver).key_down(Keys.CONTROL).send_keys("f").key_up(Keys.CONTROL).perform()
        ActionChains(self.driver).send_keys(text).send_keys(Keys.ENTER).perform()
        new_page_number = self.find_element_by_accessibility_id("PageLabelEdit").text
        self.find_element_by_accessibility_id("FindBar.QToolButton").click()
        self.assertEqual(page_number, new_page_number)

    def assert_annotation_has_text(self, text):
        ActionChains(self.driver).key_down(Keys.CONTROL).send_keys("f").key_up(Keys.CONTROL).perform()
        ActionChains(self.driver).send_keys(text).send_keys(Keys.ENTER).perform()
        self.driver.implicitly_wait(0)
        self.assertTrue(len(self.driver.find_elements(by=AppiumBy.ACCESSIBILITY_ID, value="information")) == 0)
        self.driver.implicitly_wait(10)

    @classmethod
    def setUpClass(self):
        options = AppiumOptions()
        options.set_capability("app", "org.kde.okular.desktop")

        max_attempts = 3
        attempts = 0
        while attempts < max_attempts:
            try:
                self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
                break
            except Exception as e:
                attempts += 1
                time.sleep(1)
        self.driver.implicitly_wait(10)
        self.wait = WebDriverWait(self.driver, 10)

    @classmethod
    def tearDownClass(self):
        max_attempts = 3
        attempts = 0
        while attempts < max_attempts:
            try:
                self.driver.quit()
                break
            except Exception as e:
                attempts += 1
                time.sleep(1)

    def test_a_app_installed(self):
        self.assertTrue(self.driver.is_app_installed("okular"))

    def test_b_app_runs(self):
        #na
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = os.path.abspath(temp_dir)
            current_directory = os.path.dirname(os.path.abspath(__file__))
            shutil.copy(f"{current_directory}/20yearsofKDE.pdf", temp_path)

            #01a
            self.find_element_by_accessibility_id("openButton").click()
            name_area_open = self.find_element_by_class_name("text", "")
            self.write_text(name_area_open, f"{temp_path}/20yearsofKDE.pdf")
            self.find_element_by_class_name("push button", "Open").click()
            self.wait.until(EC.presence_of_element_located((AppiumBy.CLASS_NAME, "[push button | 186]")))
            self.assertTrue(self.find_element_by_class_name("push button", "186"))

            #02a
            main = self.find_element_by_accessibility_id("QApplication.okular::Shell#1")
            ActionChains(self.driver).move_to_element(main).key_down(Keys.CONTROL).send_keys("g").key_up(Keys.CONTROL).perform()

            #02b
            go_to_page = self.find_element_by_class_name("spin button", "Page:")
            ActionChains(self.driver).move_to_element(go_to_page).send_keys("38").perform()
            self.find_element_by_class_name("push button", "OK").click()
            self.assert_current_page_has_text("Torben broke the cloning machine")

            #03a
            ActionChains(self.driver).send_keys(Keys.F6).perform()

            #03b
            ActionChains(self.driver).key_down(Keys.ALT).send_keys("1").key_up(Keys.ALT).perform()
            ActionChains(self.driver).move_to_element(self.find_element_by_accessibility_id("QMenuBar.file")).move_by_offset(180, 304).click_and_hold().move_by_offset(872, 364).release().perform()
            ActionChains(self.driver).move_to_element(self.find_element_by_accessibility_id("QMenuBar.file")).move_by_offset(400, 500).double_click().perform()

            #03c
            ActionChains(self.driver).send_keys("Very interesting text! I should read more about this topic.").perform()

            #03d
            ActionChains(self.driver).key_down(Keys.ALT).send_keys("1").key_up(Keys.ALT).perform()

            #04a
            self.open_view_menu()
            self.find_element_by_class_name("menu item", "Presentation").click()
            ActionChains(self.driver).send_keys(Keys.ENTER).perform()

            #04b
            ActionChains(self.driver).send_keys(Keys.DOWN, Keys.DOWN, Keys.DOWN, Keys.DOWN, Keys.DOWN).perform()

            #04c
            ActionChains(self.driver).send_keys(Keys.UP, Keys.UP, Keys.UP, Keys.UP, Keys.UP).perform()

            #04d
            ActionChains(self.driver).send_keys(Keys.ESCAPE).perform()
            self.assert_annotation_has_text("Very interesting text! I should read more about this topic.")

            #05a
            for _ in range(2):
                self.rotate_left()

            #05b
            for _ in range(2):
                self.rotate_right()

            #06a
            main_bar_buttons = self.driver.find_elements(by=AppiumBy.ACCESSIBILITY_ID, value="QApplication.okular::Shell#1.mainToolBar.miniBar.HoverButton")
            next_page_button = main_bar_buttons[2]
            for _ in range(5):
                next_page_button.click()
                time.sleep(0.1)
            self.assert_current_page_has_text("Re-writing the bylaws proved to be a challenge")

            #06b
            previous_page_button = main_bar_buttons[0]
            for _ in range(5):
                previous_page_button.click()
                time.sleep(0.1)
            self.assert_current_page_has_text("Torben broke the cloning machine")

            #07a
            self.open_view_menu()
            self.find_element_by_class_name("menu item", "Zoom to 100%").click()

            #07b
            zoom_in = self.find_element_by_class_name("push button", "Zoom In")
            for _ in range(5):
                zoom_in.click()

            #07c
            self.open_view_menu()
            self.find_element_by_class_name("menu item", "Fit Width").click()

            #08
            self.find_element_by_class_name("menu item", "Settings").click()
            self.find_element_by_accessibility_id("options_configure").click()
            self.find_element_by_name("Accessibility").click()
            self.find_element_by_class_name("check box", "Change colors").click()
            self.find_element_by_class_name("push button", "Apply").click()
            self.find_element_by_class_name("push button", "OK").click()

            #09a
            self.find_element_by_accessibility_id("QMenuBar.go").click()
            self.find_element_by_accessibility_id("go.go.go_goto_page").click()
            go_to_page = self.find_element_by_class_name("spin button", "Page:")

            #09b
            ActionChains(self.driver).move_to_element(go_to_page).send_keys("42").perform()
            self.find_element_by_class_name("push button", "OK").click()
            self.assert_current_page_has_text("In 1999 I was elected to the KDE")

            #10a
            ActionChains(self.driver).send_keys(Keys.F6).perform()

            #10b
            ActionChains(self.driver).key_down(Keys.ALT).send_keys("1").key_up(Keys.ALT).perform()
            (ActionChains(self.driver)
             .move_to_element(self.find_element_by_accessibility_id("QMenuBar.file"))
             .move_by_offset(180, 304)
             .click_and_hold().move_by_offset(872, 364)
             .release().perform())

            #10c
            (ActionChains(self.driver)
             .move_to_element(self.find_element_by_accessibility_id("QMenuBar.file"))
             .move_by_offset(400, 500).double_click()
             .send_keys("Again this is very interesting, should read more.").perform())
            self.assert_text_in_annotation("Again this is very interesting, should read more.")

            #10d
            ActionChains(self.driver).key_down(Keys.ALT).send_keys("1").key_up(Keys.ALT).perform()

            #11a
            self.open_view_menu()
            self.find_element_by_class_name("menu item", "Presentation").click()
            ActionChains(self.driver).send_keys(Keys.ENTER).perform()

            #11b
            ActionChains(self.driver).send_keys(Keys.ESCAPE).perform()

            #12a
            for _ in range(2):
                self.rotate_right()

            #12b
            for _ in range(2):
                self.rotate_left()

            #13a
            for _ in range(5):
                next_page_button.click()
                time.sleep(0.1)
            self.assert_current_page_has_text("Since 2010 he lives in Oslo")

            #13b
            for _ in range(5):
                previous_page_button.click()
                time.sleep(0.1)
            self.assert_current_page_has_text("Aaron couldn't be present at the meeting in Hamburg")

            #14a
            self.open_view_menu()
            self.find_element_by_class_name("menu item", "Zoom to 100%").click()

            #14b
            zoom_in = self.find_element_by_class_name("push button", "Zoom In")
            for _ in range(5):
                zoom_in.click()

            #14c
            self.open_view_menu()
            self.find_element_by_class_name("menu item", "Fit Width").click()

            #15
            self.find_element_by_class_name("menu item", "Settings").click()
            self.find_element_by_accessibility_id("options_configure").click()
            self.find_element_by_name("Accessibility").click()
            self.find_element_by_class_name("check box", "Change colors").click()
            self.find_element_by_class_name("push button", "Apply").click()
            self.find_element_by_class_name("push button", "OK").click()

            #16a
            self.open_file_menu()
            self.find_element_by_accessibility_id("file_save").click()

            #16b
            #self.open_file_menu() ////the menu is not closed by the previous action so we don't need to open it again
            self.find_element_by_accessibility_id("file_quit").click()

if __name__ == '__main__':
    unittest.main()
