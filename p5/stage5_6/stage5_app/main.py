from flask import Flask, render_template, send_from_directory, request, redirect, url_for
from flask import session, flash, send_file
import sqlite3
import json
import os, io
from urllib.request import urlopen
from urllib.error import HTTPError, URLError
import datetime
from werkzeug.utils import secure_filename
from flask import g
import platform

item_perpage = 12

app = Flask(__name__)
app.secret_key = 'helloworldlkh123lk1h3lskhf.,23.t,23'
SESSION_TYPE = 'null'
LINUX_UPLOAD_FOLDER = '.'#'static/uploads'
WINDO_UPLOAD_FOLDER = '.'#'static\\uploads'
linux_bulk = '.'#'static/admin'
windo_bulk = '.'#'static\\admin'
bulk_load = ''
ALLOWED_EXTENSIONS = set(['jpg', 'sql'])
app.config.from_object(__name__)

if platform.system() == 'Windows':
    app.config['UPLOAD_FOLDER'] = WINDO_UPLOAD_FOLDER
    app.config['BULK'] = windo_bulk
else:
    app.config['UPLOAD_FOLDER'] = LINUX_UPLOAD_FOLDER
    app.config['BULK'] = linux_bulk
	
DATABASE = 'sql/main.db'
#conn = sqlite3.connect("data.db", check_same_thread = False);
root = "root"

def make_dicts(cursor, row):
    return dict((cursor.description[idx][0], value) for idx, value in enumerate(row))

def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(DATABASE, check_same_thread = False)
    db.row_factory = make_dicts
    return db

@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()

########### all static pages beneath#########

@app.route('/')
@app.route('/index.html')
def index():
    if session.get('mysid') is None:
        session['mysid'] = -1
    ran_prods = random_products(20)
    return render_template('basic/index.html', products=ran_prods);

@app.route('/shop-right', methods=['GET'])
def shop_right():
    if session.get('logged_in'):
        if session['logged_in'] == True:
            shops = query_db("select * from shop")
            session['query'] = "select * from shop"
            #!
            page_no = request.args.get('page_no')
            pl = setupsession(session, shops, page_no, session['query'])
            return render_template('basic/shop-list.html', prevpage=pl[0], nextpage=pl[1], pagedata=pl[2]);
    flash("please login first")
    return redirect(url_for(".index"))

# this is in fact pagination
def setupsession(session, li, page_no, prev_query):
    pl = []
    print(len(li))
    if page_no is not None: #if page_no is not None, should use query to get data again
        li = query_db(prev_query)
        session['page_no'] = request.args.get('page_no')
        re = []
        minno =(int(page_no) - 1) * item_perpage - 1
        maxno = int(page_no) * item_perpage
        print(minno)
        print(maxno)
        i = 0
        for row in li:
            if i > maxno:
                break
            if i > minno and i < maxno:
                re.append(row)
            i+=1
        p = session['page_no']
        session['datasize'] = len(re)
        if re:
            session['f_item'] = re.pop(0)
        prevpage = int(page_no) - 1
        if prevpage < 1:
            prevpage = -1
        nextpage = int(page_no) + 1
        if nextpage > session['max_page']:
            nextpage = -1
        pl.append(prevpage)
        pl.append(nextpage)
        pl.append(re)
        return pl
    else:#if page_no is None, there is data in li
        session['page_no'] = 1
        max_page = len(li) / item_perpage
        if len(li) % item_perpage > 0:
            max_page += 1
        session['max_page'] = int(max_page)
        session['datasize'] = len(li)
        if li:
            session['f_item'] = li.pop(0)
        i = 0
        re = []
        for row in li:
            if i > item_perpage:
                break
            if i < item_perpage - 1:
                re.append(row)
            i += 1
        prevpage = -1
        nextpage = 2
        if nextpage > int(max_page):
            nextpage = -1
        pl.append(prevpage)
        pl.append(nextpage)
        pl.append(re)
        return pl

@app.route('/cart-right')
def cart_right():
    if session.get('logged_in'):
        if session['logged_in'] == True:
            items = query_db("select * from item natural join cart where cart.UID = ?", [session['uid']])
            total = 0
            for item in items:
                total += (item['Price'] * item['amount'])
            return render_template('basic/cart-right.html', items = items, total = total);
    flash("please login first")
    return redirect(url_for(".index"))

@app.route('/product')
def product_right():
    if session.get('logged_in'):
        if session['logged_in'] == True:
            iid = request.args.get('iid')
            product = query_db("select * from item natural join normalitem where item.iid = ?", [iid])[0]
            if not session.get('sid'):
                session['sid'] = product['SID']
            
            shop_info = query_db("select * from shop where sid = ?", [product['SID']])[0]

            if session['uid'] == shop_info['OperatedByUID'] or session['username'] == root:
                session['is_operator'] = True
            else:
                session['is_operator'] = False
            return render_template('basic/product-right.html', product=product);
    flash("please login first")
    return redirect(url_for(".index"))

@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        if password == "":
            flash("password cannot be empty")
            return render_template('basic/register.html')
        if request.form['password2'] != password:
            flash("passwords do not match")
            return render_template('basic/register.html')
        result = query_db("select count(UID) as c from User where Username = ?", [username])
        if result[0]['c'] > 0:
            flash("username has been used")
            return render_template('basic/register.html')
        conn = get_db()
        cu = conn.cursor()
        cu.execute("insert into user (Username, Password) values (? , ?)", [username, password])
        uid = int(cu.lastrowid)
        conn.commit()
        session['logged_in'] = True
        session['username'] = username
        session['uid'] = uid
        session['query'] = ""
        
        starttime = str(datetime.datetime.now())
        result = query_db("select SID from shop where operatedByUID = ?", [uid])
        endtime = str(datetime.datetime.now())
        print("register: " + starttime + "         " + endtime)
        
        if result:
            session['mysid'] = result[0]['SID']
        else:
            session['mysid'] = -1
        return redirect(url_for('.index'))
    return render_template('basic/register.html');

@app.route('/<path:path>')
def serve_style(path):
    if path.endswith('5.jpg'):
        return send_from_directory('.', path)
    return send_from_directory('static/', path)

########### all static pages above #########

def query_db(query, args=(), one=False):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    cur.close()
    return (rv[0] if rv else None) if one else rv

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        cursor = get_db().cursor()
        starttime = datetime.datetime.now()
        results = query_db("select * from user where Username=?", [username])
        endtime = datetime.datetime.now()
        print(str(starttime) + "       " + str(endtime))
        if len(results) < 1 :
            app.logger.info("no user.")
        else:
            for row in results:
                if password == row['Password']:
                    if username == root:
                        session['is_operator'] = True
                    session['logged_in'] = True
                    session['username'] = username
                    session['uid'] = row['UID']
                    session['query'] = ""
                    result = query_db("select SID from shop where operatedByUID = ?", [session['uid']])
                    if result:
                        session['mysid'] = result[0]['SID']
                    else:
                        session['mysid'] = -1
                    return redirect(url_for('.index'))
            app.logger.info("no user..")
    return redirect(url_for('.index'))

# need to specify shop id in url
@app.route('/shop-indi', methods=['GET'])
def shop_indi():
    if session.get('logged_in') is None:
        flash("please login first")
        return redirect(url_for(".index"))
    if request.method == 'GET':
        sql1 = "select * from item natural join normalitem where item.Status = 'display' and item.SID = {}"
        sql2 = "select * from item natural join normalitem where item.SID = {}"
        sql_select = ""
        sid = request.args.get('sid')
        
        if sid == "":
            return redirect(url_for('.shop_right'))
        session['sid'] = sid
        
        starttime = datetime.datetime.now()
        shop_info = query_db("select * from Shop where SID=?", [int(sid)])
        endtime = datetime.datetime.now()
        print(str(starttime) + "       " + str(endtime))
        
        shop_info = shop_info[0]
        session['shop_info'] = shop_info
        if shop_info['OperatedByUID'] == session['uid'] or session['username'] == root:
            sql_select = sql2
            session['is_operator'] = True
        else:
            sql_select = sql1
            session['is_operator'] = False
        print("sid: " + sid)
        sql_select = sql_select.format(sid)
        print("sql_select: " + sql_select)
        page_no = request.args.get('page_no')
        cursor = get_db().cursor()
        
        #if posts is already in session
        if page_no is not None:
            li = []
            pl = setupsession(session, li, page_no, session['query'])
            pl[2] = url_exist(pl[2]) 
            return render_template('basic/shop-indi.html', prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])
        #if posts is not yet in session
        else:
            session['query'] = sql_select

            starttime = datetime.datetime.now()
            results = query_db(sql_select)
            endtime = datetime.datetime.now()
            print("shop-indi: " + str(starttime) + "       " + str(endtime))
            
            pl = setupsession(session, results, None, sql_select)
            pl[2] = url_exist(pl[2]) 
            return render_template('basic/shop-indi.html', prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])

@app.route('/insert', methods=['GET', 'POST'])
def insert():
    if request.method == 'POST':
        itemname = request.form['itemname']
        brand = request.form['brand']
        color = request.form['color']
        description = request.form['description']
        price = request.form['price']
        inventory = request.form['inventory']
        
        if request.files['image'].filename != "":
            image = request.files['image']
            fn = secure_filename(image.filename)
            filename = str(session['uid'])
            filename += '{0:%Y%m%d%H%M%S}'.format(datetime.datetime.now())
            filename += '5.jpg'
            #image.save(app.config['UPLOAD_FOLDER'], filename)
            filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
            tmpfile = open(filepath, 'wb')
            image.save(filepath)
            tmpfile.close()
        conn = get_db()
        cu = conn.cursor()
        cu.execute("insert into item (SID, Name, Brand, Color, Description, imageURL, Status)\
                values (?,?,?,?,?,?,?)",\
                [session['sid'], itemname, brand, color, description, filepath, 'display'])
        iid = cu.lastrowid
        cu.execute("insert into normalitem (IID, Price, Inventory) \
                    values (?,?,?)",[iid, price, inventory]) 
        conn.commit()
        flash("insert success")
        return redirect(url_for('shop_indi', sid=session['sid']))
    return render_template('basic/insert-item.html')

@app.route('/delete', methods=['GET'])
def delete():
    if session['username'] is None:
        return redirect(url_for('.index'))
    else:
        iid = int(request.args.get('iid'))
        conn = get_db()
        cu = conn.cursor()
        if session['username'] == root:
            cu.execute("update item set Status = 'deleted' where IID = ?", (iid,))
        else:
            cu.execute("delete from item where IID = ?", (iid,))
            cu.execute("delete from normalitem where IID = ?", (iid,))
        conn.commit()
        #session['posts'] = None
        return redirect(url_for('shop_indi', sid=session['sid'])) 

@app.route('/update', methods=['POST'])
def update():
    if session['username'] is None:
        return redirect(url_for('.index'))
    else:
        iid = request.form['iid']
        obj = request.form['type']
        conn = get_db()
        cu = conn.cursor()
        if obj == "name":
            replace = request.form['name']
            cu.execute("update item set Name = ? where IID = ?", (replace, iid))
        if obj == "price":
            replace = request.form['name']
            cu.execute("update normalitem set Price = ? where IID = ?", (replace, iid))
        if obj == "brand":
            replace = request.form['name']
            cu.execute("update item set Brand = ? where IID = ?", (replace, iid))
        if obj == "color":
            replace = request.form['name']
            cu.execute("update item set Color = ? where IID = ?", (replace, iid))
        if obj == "inventory":
            replace = request.form['name']
            cu.execute("update normalitem set Inventory = ? where IID = ?", (replace, iid))
        if obj == "description":
            replace = request.form['name']
            cu.execute("update item set Description = ? where IID = ?", (replace, iid))
        conn.commit()
        return redirect(url_for("product_right", iid=iid))

@app.route('/query', methods=['GET'])
def query():
    if session.get('logged_in'):
        if session['logged_in'] == True:
            query = request.args.get('query')
            results = []
            query = "%" + query + "%"
            
            sql1 = "select * from item natural join normalItem where item.Status = 'display' and item.Brand = '{}'"
            sql2 = "select * from item natural join normalItem where item.Status = 'display' and item.Brand like '{}'"
           
            #first do a match search on brand
            #then do more general search brand
            #then do more general search on item name
            querystring = sql1.format(request.args.get('query'))
            
            starttime = datetime.datetime.now()
            brands = query_db(querystring)
            endtime = datetime.datetime.now()
            print(str(starttime) + "       " + str(endtime))
            
            if len(brands) > 0:
                results = brands
            else:
                print("no match!!!!!!")
                querystring = sql2.format(query)
                brands = query_db(querystring)
                
                if len(brands) > 0:
                    results = brands
                else:
                    querystring = "select * from item natural join normalItem where item.Status = 'display' and item.Name like '{}'".format(query)
                    results = query_db(querystring)
            #!
            session['query'] = querystring
            pl = setupsession(session, results, None, querystring)
            print("in query(): len(pl[2]): " + str(len(pl[2])))
            return render_template("basic/product-results.html", prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])
    #if not logged in
    else:
        flash("please login first")
        return redirect(url_for(".index"))

@app.route('/product-result', methods=['GET'])
def get_products():
    if session.get('logged_in') is None:
        flash("please login first")
        return redirect(url_for(".index"))
    if request.method == 'GET':
        page_no  = request.args.get('page_no')
        if page_no == "":
            #TODO throw random products
            pl = setupsession(session, random_products(200), None)
            return render_template("basic/product-results.html", prevpage=pl[0], nextpage=pl[1])
        else:
            #!
            print("in get_products: "+session['query'])
            li = []
            pl = setupsession(session, li, page_no, session['query'])
            return render_template("basic/product-results.html", prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])
    
def random_products(num):
    re = []
    results = query_db("select IID from item natural join normalitem  where item.imageURL != '' order by random() limit ?", [num])
    conn = get_db()
    cu = conn.cursor()
    for result in results:
        tmp = cu.execute("select * from item natural join normalitem where IID = ?", [result['IID']])
        tmp2 = tmp.fetchall()
        re.append(tmp2[0])
    print("num of results")
    print(len(re))
    print(num)
    return re

@app.route('/bulk-load', methods=['GET', 'POST'])
def bulk_load():
    if request.method == 'POST':
        if request.files['sql'].filename != "":
            sqlfile =  request.files['sql']
            fn = secure_filename(sqlfile.filename)
            filename = '{0:%Y%m%d%H%M%S}'.format(datetime.datetime.now())

            filename += ".sql"
            #sqlfile.save(app.config['BULK'], filename)
            filepath = os.path.join(app.config['BULK'], filename)
            #tmpfile = open(filepath, 'wb')
            sqlfile.save(filepath)
            #tmpfile.close()
            
            sqlfile = open(filepath, encoding='utf-8')
            conn = get_db()
            cu = conn.cursor()
            script = ""
            for row in sqlfile:
                script+=row
            cu.execute("begin")
            try:
                cu.executescript(script)
            except Exception as e:
                for row in sqlfile:
                    print(row)
                print("error")
                app.logger.info(e)
                conn.rollback()
                return render_template('basic/fail.html', errmsg=str(e))
            conn.commit()
            cu.close()
            for row in sqlfile:
                if row != "":
                    print(row)
                    #TODO exceptions 
            return render_template('basic/success.html')
    else:
        return render_template('basic/bulk.html')

@app.route('/checkout-right', methods=['GET', 'POST'])
def checkout():
    if request.method == 'POST':
        firstname = request.form['billing_first_name']
        lastname = request.form['billing_last_name']
        address1 = request.form['billing_address_1']
        address2 = request.form['billing_address_2']
        city = request.form['billing_city']
        country = request.form['billing_state']
        postcode = request.form['billing_postcode']
        savedcards = query_db("select * from PaymentMethod where UID = ?", [session['uid']])
        usingnew = 1
        using = request.form['billing_cardno']
        for card in savedcards:
            option = request.form.get(card['NickName'])
            if option:
                using = card
                usingnew = 0
                break
        time = datetime.datetime.now()
        status = "ongoing"
        uid = session['uid']
        addr = firstname + lastname + "," + address1 + "," + address2 + "," + city + "," + postcode + "," + country
        conn = get_db()
        cu = conn.cursor()
        cu.execute("insert into Orders (Date, Status, ShippingAddr, BuyerUID) \
                    values(?,?,?,?)",\
                    [time, status, addr, session['uid']])
        if usingnew == 1:
            newname = request.form['billing_cardname']
            cu.execute("insert into PaymentMethod (UID, NickName, FirstEightCardNum, CardType) \
                        values(?,?,?,?)", [session['uid'], newname, using, "cheque"])
        conn.commit()
        # clear cart
        # maybe do something else? no time for it...
        uid = session['uid']
        conn = get_db()
        cu = conn.cursor()
        cu.execute("delete from cart where UID = ?", [uid])
        conn.commit()
        return render_template("basic/thankyou.html")
    else:
        if session.get('logged_in'):
            if session['logged_in'] == True:
                cards = query_db("select * from PaymentMethod where UID = ?", [session['uid']])
                print("hello")
                print(cards)
                # get total
                items = query_db("select * from item natural join cart where cart.UID = ?", [session['uid']])
                total = 0
                for item in items:
                    total += (item['Price'] * item['amount'])
                return render_template('basic/checkout-right.html', cards=cards, total = total, items = items)
        else:
            flash("please login first")
            return redirect(url_for(".index"))

@app.route('/add-to-cart', methods=['GET','POST'])
def addtocart():
    if session.get('logged_in'):
        conn = get_db()
        cu = conn.cursor()
        uid = session['uid']
        iid = 0
        updatesql = "update cart set amount = ? where UID = ? and IID = ?"
        insertsql = "insert into cart (UID, IID, amount, Price) values (?,?,?,?)"
        queryprice = "select price from normalitem where IID = ?"
        if request.method == 'POST':
            iid = request.form['iid']
            re = cu.execute("select * from cart where UID = ? and IID = ?", [uid, iid])
            count = re.fetchall()
            pri = cu.execute(queryprice, [iid]).fetchall()[0]['Price']
            quan = request.form['product_quantity']
            print(quan)
            if len(count) > 0:
                quan2 = count[0]['amount']
                cu.execute(updatesql, [str(quan+quan2), uid, iid])    
            else:
                cu.execute(insertsql, [uid, iid, quan, pri])
        else:
            iid = request.args.get('iid')
            re = cu.execute("select * from cart where UID = ? and IID = ?", [uid, iid])
            count = re.fetchall()
            pri = cu.execute(queryprice, [iid]).fetchall()[0]['Price']
            print(len(count))
            if len(count) > 0:
                quan = count[0]['amount']
                cu.execute(updatesql, [quan+1, uid, iid])
            else:
                cu.execute(insertsql, [uid, iid, 1, pri])
        conn.commit()
        Sid = query_db("select SID from item where IID = ?", [iid])
        return redirect(url_for("shop_indi", sid=Sid[0]['SID']))
    else:
        flash("please login first")
        return redirect(url_for(".index"))

@app.route('/shop-filter', methods=['POST'])
def filter():
    if request.method == 'POST':
        lower = request.form['lower']
        higher = request.form['higer']
        print("price filter values: ")
        print(lower)
        print(higher)
        session['price_l'] = lower
        session['price_h'] = higher
        querystring = "select * from item natural join normalitem where SID = {} and \
                       Price > {} and Price < {}".format(session['sid'], lower, higher)
        print("in filter: " + querystring)
        results = query_db("select * from item natural join normalitem \
                            where SID = ? and Price > ? and Price < ?", \
                            [session['sid'], lower, higher])
        #session['posts'] = results
        session['query'] = querystring
        #!
        pl = setupsession(session, results, None, querystring)
        pl[2] = url_exist(pl[2])
        return render_template("basic/shop-indi.html", prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])
    else:
        return redirect(url_for('.index'))

def url_exist(li):
    for item in li:
        try:
            url = item['imageURL'];
            urlopen(url)
        except HTTPError:
            item['imageURL'] = "images/shop/noimage.jpg"
        except URLError:
            item['imageURL'] = "images/shop/noimage.jpg"
        except Exception as e:
            print(e)
    return li;

@app.route('/shop-search', methods=['GET'])
def shop_search():
    if request.method == 'GET':
        sid = request.args.get('sid')
        if sid == "" or sid is None:
            return redirect(url_for('.shop_right'))
        shop_info = query_db("select * from Shop where SID=?", [int(sid)])
        shop_info = shop_info[0]
        session['shop_info'] = shop_info
        if shop_info['OperatedByUID'] == session['uid'] or session['username'] == root:
            session['is_operator'] = True
        else:
            session['is_operator'] = False
        query = request.args.get('search')
        query = "%" + query + "%"
        
        sqlb1 = "select * from item natural join normalItem where item.Status = 'display' and item.SID = {} and item.Brand like '{}'"
        sqlb2 = "select * from item natural join normalItem where item.SID = '{}' and item.Brand like '{}'"
        sqln1 = "select * from item natural join normalItem where item.Status = 'display' and item.SID = {} and item.Name like '{}'" 
        sqln2 = "select * from item natural join normalItem where item.SID = {} and item.Name like '{}'" 
        
        sqlb = ""
        sqln = ""
        if session['is_operator'] == True:
            sqlb = sqlb2
            sqln = sqln2
        else:
            sqlb = sqlb1
            sqln = sqln1
        results = []
        #!
        querystring = sqlb.format(session['sid'], query) 
        brands = query_db(querystring)
        if len(brands) > 0:
            results = brands
        else:
            querystring = sqln.format(session['sid'], query)
            results = query_db(querystring)
        session['query'] = querystring
        pl = setupsession(session, results, None, querystring)
        pl[2] = url_exist(pl[2]) 
        return render_template("basic/shop-indi.html", prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])
    return redirect(url_for('.index'))

@app.route('/prod-filter', methods=["POST"])
def prod_filter():
    if request.method == 'POST':
        lower = int(request.form['lower'])
        higher = int(request.form['higer'])
        session['price_l'] = lower
        session['price_h'] = higher
        ori_query = session['query']
        addi = "  and normalItem.Price > {} and normalItem.Price < {}".format(lower, higher)
        session['query'] = ori_query + addi
        ori = query_db(session['query'])
        results = []
        for item in ori:
            if item['Price'] > lower and item['Price'] < higher:
                results.append(item)
        #session['posts'] = results
        #!
        pl = setupsession(session, results, None, session['query'])
        return render_template("basic/product-results.html", prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])
    return redirect(url_for('.index'))
    
@app.route('/remove-cart', methods=["GET"])
def cart_remove():
    if request.method == 'GET':
        iid = request.args.get('iid');
        uid = session['uid']
        conn = get_db()
        cu = conn.cursor()
        cu.execute("delete from cart where UID = ? and IID = ?", [uid, iid])
        conn.commit()
        items = query_db("select * from item natural join cart where cart.UID = ?", [session['uid']])
        total = 0
        for item in items:
            total += (item['Price'] * item['amount'])
        return render_template('basic/cart-right.html', items = items, total = total);

@app.route('/price-filter', methods=["POST"])
def price_filter():
    if request.method == 'POST':
        lower = int(request.form['lower'])
        higher = int(request.form['higer'])
        session['price_l'] = lower
        session['price_h'] = higher
        addi = " select * from item natural join normalitem where normalItem.Price > {} and normalItem.Price < {}".format(lower, higher)
        session['query'] = addi
        ori = query_db(session['query'])
        results = []
        for item in ori:
            if item['Price'] > lower and item['Price'] < higher:
                results.append(item)
        #session['posts'] = results
        #!
        pl = setupsession(session, results, None, session['query'])
        return render_template("basic/product-results.html", prevpage=pl[0], nextpage=pl[1], pagedata=pl[2])
    return redirect(url_for('.index'))

@app.route('/logout')
def logout():
    uid = session['uid']
    session.clear()
    conn = get_db()
    cu = conn.cursor()
    cu.execute("delete from cart where UID = ?", [uid])
    conn.commit()
    return redirect(url_for('.index'))

@app.route('/test')
def test():
    arr = ['1','2','3']
    return render_template('test.html', arr=arr)

#get_db().commit()
if __name__ == "__main__":
    app.debug = True
    app.run()




#done 1. shops list, using Shop-right as template
#done 2. page for individual item
#done 3. page for insertion
#done 4. index page's item pictures
#done 5. shopping cart
#done 6. bulk loading interface
#done 7. search
#done 8. data filters
#done 9. checkout


