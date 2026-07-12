Buy companies then sell them when you think you did well
Deck of spells that refresh when you finish your hand

- [X] Money
- [X] 1 money equals 1k dollars
- [X] Other building info display
- [X] Pick random building model for different buildings
- [X] Pick random name for building models
- [X] Merge UI button
- [X] Spells UI
- [X] Spells Backfire
- [X] Floor Texture
- [X] Sky and day cycle
- [X] Spells bucket
- [X] Money UI
- [X] Goal amounts
- [ ] Basic dialog UI
- [ ] Intro scene
- [X] Save/Load save data
- [ ] Make Backfire text fall
- [ ] Make animated text objects that can be spawned to show money earned or spent
- [ ] Animated 3D characters for UI

- [X] Acquire only if your value is greater than
- [ ] Spell particles (Cylinder with transparent light texture??)
- [ ] Increase/Decrease money animations
- [x] Pick building model based on performance metrics
- [X] Create building tiers for different ranges of value, creating a beleivable ceiling for value
- [ ] Music
- [ ] Main menu
- [ ] Sound effects for each spell
- [ ] Smooth snapping of camera
- [ ] Lose condition
- [ ] Cooldown for spawning new company


Spells
- [X] Sabotage electricity - Reduce income
- [X] Luck spell - Increase income temporarily
- [X] Negative PR - Reduce income, small chance to greatly increase income
- [X] Cook their books - Use your magic to cook their books, if they get caught - 90% value, if they don't + 20% value
- [X] Delete - Deletes the company and spawns a new one

Stretch
- [ ] Settings menu
- [ ] Don't pick same company name twice
- [ ]


Volatility = (income_scale + expense_scale) / 2
Bias = (income_scale - expense_scale) / volatility
Spike = 10x income or expense


Tiers
- 0: 0k - 100k
- 1: 100k - 250k
- 2: 250k - 500k
- 3: 500k - 1M
- 4: 1M - 2M
- 5: 2M - 5M



Formulas:
Sigmoid: f\left(x\right)\ =\ \frac{1}{1+e^{-a\left(x+b\right)}}
Cos Sigmoid: \frac{\left(1-\cos\left(\pi x\right)\right)}{2}