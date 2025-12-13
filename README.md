### Coin-helper
This project will help you carefully manage your own finances and your family's finances

Designed by [mnonessss](https://github.com/mnonessss) and [grecha-top](https://github.com/grecha-top)

You can use it via link: [coin-helper](https://coin-helper.ru)

When you login the first time, you will see only your own finances. But if you have invitation in a family, you can take it. After that you can manage your family's finances too.

**To invite somebody** in your family you can push a button "Создать семью" in your home page. **Warning** Now you can't join a family via an email. You need to register using an email that inviter entered to invite you in a fammily. After that in home page you can accept this invitation.

[mnonessss](https://github.com/mnonessss) created:
1. all models using special command by Drogon Framework.
2. Controllers:
	- UserController
	- AccountController
	- BudgetController
	- CategoryController
3. views for these controllers. Designed utils for encryption and verifying passwords.

[grecha-top](https://github.com/grecha-top) created:
1. Postgres database
2. Controllers:
	- PageController
	- TransactionsController
	- TransferController 
3. views for these controllers.

Together we created:
1. CMakeLists.txt
2. build/ 
3. config.json
4. .gitignore
5. main.cc.
6. We set up VDS and deployed our website.
