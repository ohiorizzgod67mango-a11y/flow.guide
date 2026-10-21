from playwright.sync_api import sync_playwright
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parent
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,args=['--no-sandbox'])
 page=browser.new_page(viewport={'width':1440,'height':1100})
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.goto('http://127.0.0.1:8080')
 page.wait_for_function("document.querySelector('#checkState').textContent==='PASS'")
 page.click('#tap')
 page.wait_for_function("document.querySelector('#hold').textContent==='4.17 ms'")
 assert page.locator('#corrected').inner_text()=='1'
 print('short tap browser regression passed')
 page.click('#enabled')
 page.click('#tap')
 page.wait_for_function("document.querySelector('#hold').textContent==='4.17 ms'")
 assert page.locator('#corrected').inner_text()=='0'
 print('bypass browser regression passed')
 page.evaluate('running=false;elapsed=2.1')
 page.wait_for_function('lastResult && lastResult.duration===2.1')
 assert page.evaluate('JSON.stringify(physics(deliveries,2.1))===JSON.stringify(physics(rawDeliveries,2.1))')
 print('bypass trajectory equality passed')
 page.click('#enabled');page.click('#wave');page.click('#demo')
 page.wait_for_function("Number(document.querySelector('#corrected').textContent)>0")
 print('wave demo passed')
 page.click('#reset');page.click('#play')
 page.locator('#game').focus()
 page.keyboard.down('Space');page.wait_for_timeout(100);page.keyboard.up('Space')
 page.wait_for_function("Number(document.querySelector('#traceStatus').textContent.split(' ')[0])>=2")
 print('live keyboard smoke passed')
 page.click('#reset');page.click('#play')
 box=page.locator('#game').bounding_box()
 page.mouse.move(box['x']+150,box['y']+150);page.mouse.down();page.wait_for_timeout(80);page.mouse.up()
 page.wait_for_function("Number(document.querySelector('#traceStatus').textContent.split(' ')[0])>=2")
 print('live pointer smoke passed')
 page.click('#cube');page.click('#reset')
 page.screenshot(path=str(ROOT/'workshop-preview.png'),full_page=True)
 page.set_viewport_size({'width':390,'height':844})
 assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
 print('mobile layout passed')
 assert not errors,errors
 print('no browser runtime errors')
 browser.close()
