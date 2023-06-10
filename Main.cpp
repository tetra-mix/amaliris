# include <Siv3D.hpp> // OpenSiv3D v0.6.3

const int DrawChipNumX = 16;
const int DrawChipNumY = 16;
const int ChipSize = 32;
const int ChipNumX = 8;
const int ChipNumY = 8;
const int CharaSizeX = 20;
const int CharaSizeY = 28;

class Charactor
{
public:
	std::string name = "";
	Point cell = Point(64, 64);
	double walkSpeed = 4.0;
	FilePath textureImage = U"img/character1.png";
	double scale = 2.0;
	Texture texture = Texture(textureImage);

	// コンストラクタ
	Charactor(std::string name, Point cell, double walkSpeed, Grid<int> collision, FilePath textureImage, double scale)
		: name(name), cell(cell), walkSpeed(walkSpeed), collision_(collision), textureImage(textureImage), scale(scale) {
		// 次に進むセルを現在のセルに
		nextCell_ = cell;
	}

	// レンダリング（描画関連）
	virtual void render()
	{
		// 歩行中の場合
		if (cell != nextCell_)
		{
			// 歩行の進捗を進める
			walkProgress_ += Scene::DeltaTime() * walkSpeed;

			// 歩行の進捗が1.0以上になったら
			if (walkProgress_ >= 1.0)
			{
				// 現在の位置を移動しようとしている位置にする
				cell = nextCell_;
				walkProgress_ = 1.0;
			}
		}

		// テクスチャ拡大描画時に、綺麗に表示されるようにする
		// (フィルタリングしないサンプラーステートを適用)
		ScopedRenderStates2D renderState(SamplerState::ClampNearest);

		// 小数も含めたセル座標
		const Vec2 cellProgress = cell.lerp(nextCell_, walkProgress_);
		// セル座標を描画座標に変換
		const Vec2 pos = (cellProgress * ChipSize) + Vec2(ChipSize / 2, ChipSize / 2);

		// 足先
		//   -1: 右
		//    0: 中立
		//    1: 左
		int playerFoot = 0;
		if (walkProgress_ < 0.5)
		{
			playerFoot = -1;
		}
		else if (walkProgress_ < 1.0)
		{
			playerFoot = 1;
		}

		// キャラクターを描画
		texture(
			startPosOfCharactor_[direction_].movedBy(playerFoot * charaSizeX_, 0),
			charaSizeX_,
			charaSizeY_
		)
			.scaled(scale)
			.draw(
				Arg::bottomCenter(
					pos.movedBy(0, 14)
				)
			);
	}

	// protected
	//   派生クラス（継承元のクラス）からはアクセスでき、クラス外からはアクセスできない
protected:
	Point nextCell_;
	double walkProgress_;
	// direction_ は直接配列の要素数指定に入れるので、コンパイル時に
	// 予め取り得る値を入れておかないとエラーが発生する
	// (int direction_; しただけでは、directionの値は0にならない (不定の値だから))
	int direction_ = 0;
	Grid<int> collision_;

	void move_(Point toCell, Point exceptCell)
	{
		// 行き先のセルを次に進むセルにする
		nextCell_ = toCell;

		// マップの範囲外に移動しようとしているとき、マップの範囲内に収める
		//   Clamp関数… 引数1の数値を引数2～引数3の範囲に収める
		nextCell_.x = Clamp(
			nextCell_.x,
			0,
			static_cast<int>(collision_.width() - 1)
		);
		nextCell_.y = Clamp(
			nextCell_.y,
			0,
			static_cast<int>(collision_.height() - 1)
		);

		// 通行できない場所や、プレイヤーの場所(exceptCell)に
		// 移動しようとしているとき、移動しない
		if (collision_[nextCell_] != 0 || exceptCell == nextCell_)
		{
			nextCell_ = cell;
		}
		else if (cell != nextCell_)
		{
			// 歩行開始
			walkProgress_ = 0.0;
		}
	}

	// private
	//   クラス外からはアクセスできず、派生クラス（継承元のクラス）からもアクセスできない
private:
	// 扱うキャラデータの1タイルの大きさ (px)
	const int charaSizeX_ = 32;
	const int charaSizeY_ = 32;

	// { 北, 東, 南, 西 }
	const Array<Vec2> startPosOfCharactor_ = {
		Vec2(32, 96),
		Vec2(32, 64),
		Vec2(32, 0),
		Vec2(32, 32)
	};
};

class NPC : public Charactor
{
public:
	double walkInterval = 2.0;
	std::vector<std::string> message;

	// コンストラクタ
	NPC(std::string name, Point initCell, double walkSpeed, double walkInterval, Grid<int> collision, FilePath textureImage, double scale, std::vector<std::string> message)
		: Charactor(name, initCell, walkSpeed, collision, textureImage, scale), walkInterval(walkInterval), message(message) {}

	// 自動コントロール
	void autoControl(Point playerPos)
	{
		// 以下の条件に全て当てはまったらzzzzzzz
		// - 歩行中ではない
		// - 歩行間隔(インターバル)が指定した秒数より大きい
		// - 半分の確立でtrueになる関数でtrueが出たとき
		if (
			cell == nextCell_
			&& Scene::Time() - lastMoved_ >= walkInterval
			&& RandomBool()
			)
		{
			// ランダムで進む方向を決める
			//   0: 北
			//   1: 東
			//   2: 南
			//   3: 西
			direction_ = Random(0, 3);

			// 移動方向に対して移動するように指示を出す
			//   継承先のmove_メソッドを呼び出す
			switch (direction_)
			{
			case 0:
				move_(Point(cell.x, cell.y - 1), playerPos);
				break;
			case 1:
				move_(Point(cell.x + 1, cell.y), playerPos);
				break;
			case 2:
				move_(Point(cell.x, cell.y + 1), playerPos);
				break;
			case 3:
				move_(Point(cell.x - 1, cell.y), playerPos);
				break;
			}

			// 最後に歩行した時刻を現在時刻にする
			lastMoved_ = Scene::Time();
		}
	}

	// private
	//   クラス外からはアクセスできず、派生クラス（継承元のクラス）からもアクセスできない
private:
	double lastMoved_ = 0.0 - walkInterval;
};

class Player : public Charactor
{
public:
	// コンストラクタ
	Player(std::string name, Point initCell, double walkSpeed, Grid<int> collision, FilePath textureImage, double scale, Camera2D* camera)
		: Charactor(name, initCell, walkSpeed, collision, textureImage, scale), camera_(camera) {}

	// 操作
	void control(Array<Point> exceptCells)
	{
		// プレイヤーが移動中でない場合、矢印キーでの操作を許可する
		if (cell == nextCell_)
		{
			//  direction_
			//    0 | 北, 北東, 東, 南東, 南, 南西, 西, 北西 | 7
			if (KeyLeft.pressed() && !KeyRight.pressed())
			{
				direction_ = 6;
				--nextCell_.x;
				if (KeyUp.pressed() && !KeyDown.pressed())
				{
					direction_ = 7;
					--nextCell_.y;
				}
				else if (KeyDown.pressed() && !KeyUp.pressed())
				{
					direction_ = 5;
					++nextCell_.y;
				}
			}
			else if (KeyRight.pressed() && !KeyLeft.pressed())
			{
				direction_ = 2;
				++nextCell_.x;
				if (KeyUp.pressed() && !KeyDown.pressed())
				{
					direction_ = 1;
					--nextCell_.y;
				}
				else if (KeyDown.pressed() && !KeyUp.pressed())
				{
					direction_ = 3;
					++nextCell_.y;
				}
			}
			else if (KeyUp.pressed() && !KeyDown.pressed())
			{
				--nextCell_.y;
				direction_ = 0;
				if (KeyLeft.pressed() && !KeyRight.pressed())
				{
					direction_ = 7;
					--nextCell_.x;
				}
				else if (KeyRight.pressed() && !KeyLeft.pressed())
				{
					direction_ = 1;
					++nextCell_.x;
				}
			}
			else if (KeyDown.pressed() && !KeyUp.pressed())
			{
				direction_ = 4;
				++nextCell_.y;
				if (KeyLeft.pressed() && !KeyRight.pressed())
				{
					direction_ = 5;
					--nextCell_.x;
				}
				else if (KeyRight.pressed() && !KeyLeft.pressed())
				{
					direction_ = 3;
					++nextCell_.x;
				}
			}

			// マップの範囲外に移動しようとしているとき、マップの範囲内に収める
			//   Clamp関数… 引数1の数値を引数2～引数3の範囲に収める
			nextCell_.x = Clamp(
				nextCell_.x,
				0,
				static_cast<int>(collision_.width() - 1)
			);
			nextCell_.y = Clamp(
				nextCell_.y,
				0,
				static_cast<int>(collision_.height() - 1)
			);

			// 通行できない場所もしくは他のNPCの場所(exceptCells)に
			// 移動しようとしているとき、移動しない
			if (collision_[nextCell_] != 0 || isContainPoint_(exceptCells, nextCell_))
			{
				nextCell_ = cell;
			}
			else if (cell != nextCell_)
			{
				// 歩行開始
				walkProgress_ = 0.0;
			}
		}
	}

	// 向いている方向から見て一つ前の座標
	Point forwardCell()
	{
		// { 北, 北東, 東, 南東, 南, 南西, 西, 北西 }
		switch (direction_)
		{
		case 0:
			return Point(cell.x, cell.y - 1);
		case 1:
			return Point(cell.x + 1, cell.y - 1);
		case 2:
			return Point(cell.x + 1, cell.y);
		case 3:
			return Point(cell.x + 1, cell.y + 1);
		case 4:
			return Point(cell.x, cell.y + 1);
		case 5:
			return Point(cell.x - 1, cell.y + 1);
		case 6:
			return Point(cell.x - 1, cell.y);
		case 7:
			return Point(cell.x - 1, cell.y - 1);
		}
	}

	// 会話
	void talkTo(NPC& npc)
	{
		// そのNPCが持っているメッセージの総数
		int messageNum = npc.message.size();
		// その中から一つ選ぶ
		int choice = Random(messageNum - 1);
		// 画面に表示
		Print(Unicode::Widen(npc.name));
		Print(Unicode::Widen(npc.message[choice]));
		Print();
	}

	// レンダリング（描画関連）
	void render() override
	{
		// 歩行中の場合
		if (cell != nextCell_)
		{
			// 歩行の進捗を進める
			walkProgress_ += Scene::DeltaTime() * 4.0;
			camera_->jumpTo(
				(cell.lerp(nextCell_, walkProgress_) * ChipSize
					+ Vec2(ChipSize / 2, ChipSize / 2)).asPoint(),
				1.0
			);

			// 歩行の進捗が1.0以上になったら
			if (walkProgress_ >= 1.0)
			{
				// 現在の位置を移動しようとしている位置にする
				cell = nextCell_;
				walkProgress_ = 1.0;
			}
		}

		// テクスチャ拡大描画時に、綺麗に表示されるようにする
		// (フィルタリングしないサンプラーステートを適用)
		ScopedRenderStates2D renderState(SamplerState::ClampNearest);

		// 足先
		//   -1: 右
		//    0: 中立
		//    1: 左
		int playerFoot = 0;
		if (walkProgress_ < 0.5)
		{
			playerFoot = -1;
		}
		else if (walkProgress_ < 1.0)
		{
			playerFoot = 1;
		}

		// プレイヤーを描画
		texture(
			startPosOfCharactor_[direction_].movedBy(playerFoot * charaSizeX_, 0),
			charaSizeX_,
			charaSizeY_
		)
			.scaled(scale)
			.draw(
				Arg::bottomCenter(
					Vec2(512 / 2, 512 / 2).movedBy(0, 14)
				)
			);
	}

	// private
	//   クラス外からはアクセスできず、派生クラス（継承元のクラス）からもアクセスできない
private:
	// 扱うキャラデータの1タイルの大きさ (px)
	const int charaSizeX_ = 20;
	const int charaSizeY_ = 28;

	// { 北, 北東, 東, 南東, 南, 南西, 西, 北西 }
	const Array<Vec2> startPosOfCharactor_ = {
		Vec2(20, 84),
		Vec2(80, 84),
		Vec2(20, 56),
		Vec2(80, 28),
		Vec2(20, 0),
		Vec2(80, 0),
		Vec2(20, 28),
		Vec2(80, 56)
	};

	Camera2D* camera_;

	// そのセル座標の配列の中に特定の座標が含まれていればtrue
	bool isContainPoint_(Array<Point> targetArray, Point targetPoint)
	{
		for (auto point : targetArray)
		{
			if (point == targetPoint)
			{
				return true;
			}
		}
		return false;
	}
};



Grid<int> LoadCSV(const FilePath& path)
{
	const CSV csv(path);
	if (!csv)
	{
		throw Error(U"CSVの読み込みに失敗しました。");
	}
	const size_t xCount = csv.columns(0);
	const size_t yCount = csv.rows();
	Grid<int> map(xCount, yCount);
	for (size_t y = 0; y < yCount; ++y)
	{
		for (size_t y = 0; y < yCount; ++y)
		{
			for (size_t x = 0; x < xCount; ++x)
				map[y][x] = csv.get<int>(y, x);
		}
	}
	return map;
}

void DrawMapChips(const Grid<int>& grid, const Texture& texture)
{
	for (size_t y = 0; y < grid.height(); ++y)
	{
		for (size_t x = 0; x < grid.width(); ++x)
		{
			const int mapChip = grid[y][x];
			if (mapChip == -1)
			{
				continue;
			}
			const int chipX = (mapChip % ChipNumX) * ChipSize;
			const int chipY = (mapChip / ChipNumY) * ChipSize;
			texture(chipX, chipY, ChipSize, ChipSize)
				.draw(x * ChipSize, y * ChipSize);
		}
	}
}
void Main()
{
	Window::Resize(512, 512);
	Scene::SetBackground(Color(5, 25, 75));
	Texture forestTile(U"img/map.png");
	const Grid<int> mapLayer1 = LoadCSV(U"csv/map_layer1.csv");
	const Grid<int> mapLayer2 = LoadCSV(U"csv/map_layer2.csv");
	const Grid<int> mapLayer3 = LoadCSV(U"csv/map_layer3.csv");
	const Grid<int> mapCollision = LoadCSV(U"csv/map_collision.csv");

	Camera2D camera(
		(Point(64, 54) * ChipSize + Vec2(ChipSize / 2, ChipSize / 2)).asPoint(),
		1.0,
		Camera2DParameters::NoControl()
	);

	Player player(
		"シブ君",
		Point(64, 54),
		4.0,
		mapCollision,
		U"img/Siv3D-kun.png",
		2.5,
		&camera
	);
	Array<NPC> npcs = {
		NPC(
			"スタルテ君",
			Point(64, 40),
			3.5,
			2.0,
			mapCollision,
			U"img/character1.png",
			1.5,
			{
				"ここはとあるテスト攻略のために作られた世界だよ。",
				"テスト習慣に何やってるんだろうね。",
				"首都はスタルテというよ。",
				"国旗にはアマリリスの花が描かれているよ"
			}
		)
	};

	while (System::Update())
	{
		Array<Point> npcsCell;

		for (auto& npc : npcs)
		{
			// 配列のそのNPCのセル座標を挿入
			npcsCell << npc.cell;
			// NPCは自律的に行動
			npc.autoControl(player.cell);

			if (player.forwardCell() == npc.cell)
			{
				if (KeyEnter.down())
				{
					player.talkTo(npc);
				}
			}
		}
		player.control(npcsCell);

		camera.update();
		{
			// 2Dカメラの設定から Transformer2D を作成
			//   カメラが移動すると共に、このスコープ(このかもめ括弧内)の
			//   オブジェクトの描画座標が動的に変わるようになる
			const auto t = camera.createTransformer();

			// 1次レイヤー (地面) の描画
			DrawMapChips(mapLayer1, forestTile);
			// 2次レイヤー (装飾物1) の描画
			DrawMapChips(mapLayer2, forestTile);
			// 3次レイヤー (装飾物2) の描画
			DrawMapChips(mapLayer3, forestTile);
			// NPCを描画
			for (auto& npc : npcs)
			{
				npc.render();
			}
		}

		// プレイヤーを描画
		player.render();
	}
}

//
// = アドバイス =
// Debug ビルドではプログラムの最適化がオフになります。
// 実行速度が遅いと感じた場合は Release ビルドを試しましょう。
// アプリをリリースするときにも、Release ビルドにするのを忘れないように！
//
// 思ったように動作しない場合は「デバッグの開始」でプログラムを実行すると、
// 出力ウィンドウに詳細なログが表示されるので、エラーの原因を見つけやすくなります。
//
// = お役立ちリンク | Quick Links =
//
// Siv3D リファレンス
// https://zenn.dev/reputeless/books/siv3d-documentation
//
// Siv3D Reference
// https://zenn.dev/reputeless/books/siv3d-documentation-en
//
// Siv3D コミュニティへの参加
// Slack や Twitter, BBS で気軽に質問や情報交換ができます。
// https://zenn.dev/reputeless/books/siv3d-documentation/viewer/community
//
// Siv3D User Community
// https://zenn.dev/reputeless/books/siv3d-documentation-en/viewer/community
//
// 新機能の提案やバグの報告 | Feedback
// https://github.com/Siv3D/OpenSiv3D/issues
//
